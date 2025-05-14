#include "evas_common_private.h" /* Also includes international specific stuff */
#include "evas_private.h"
#include "evas_blend_private.h"

#include "region.h"

#include <software/Ector_Software.h>
#include "evas_ector_software.h"
#include "draw.h"
#include "evas_filter_private.h"

#if defined HAVE_DLSYM && ! defined _WIN32
# include <dlfcn.h>      /* dlopen,dlclose,etc */
#endif

#ifdef _WIN32
# include <evil_private.h> /* dlsym */
#endif

#if defined HAVE_DLSYM
# define EVAS_GL 1
# define EVAS_GL_NO_GL_H_CHECK 1
# include "Evas_GL.h"

#else
# warning software_generic will not be able to have Evas_GL API.
#endif

#include "Evas_Engine_Software_Generic.h"
#include "evas_native_common.h"
#include "filters/evas_engine_filter.h"

#ifdef EVAS_GL
//----------------------------------//
// OSMesa...

#define OSMESA_MAJOR_VERSION 6
#define OSMESA_MINOR_VERSION 5
#define OSMESA_PATCH_VERSION 0

/*
 * Values for the format parameter of OSMesaCreateContext()
 * New in version 2.0.
 */
#define OSMESA_COLOR_INDEX	GL_COLOR_INDEX
#define OSMESA_RGBA		GL_RGBA
#define OSMESA_BGRA		0x1
#define OSMESA_ARGB		0x2
#define OSMESA_RGB		GL_RGB
#define OSMESA_BGR		0x4
#define OSMESA_RGB_565		0x5


/*
 * OSMesaPixelStore() parameters:
 * New in version 2.0.
 */
#define OSMESA_ROW_LENGTH	0x10
#define OSMESA_Y_UP		0x11


/*
 * Accepted by OSMesaGetIntegerv:
 */
#define OSMESA_WIDTH		0x20
#define OSMESA_HEIGHT		0x21
#define OSMESA_FORMAT		0x22
#define OSMESA_TYPE		0x23
#define OSMESA_MAX_WIDTH	0x24  /* new in 4.0 */
#define OSMESA_MAX_HEIGHT	0x25  /* new in 4.0 */

/* Required for orient */
#define TILE 32


typedef void (*OSMESAproc)();
typedef struct osmesa_context *OSMesaContext;
#endif

typedef struct _Render_Engine_GL_Surface    Render_Engine_GL_Surface;
typedef struct _Render_Engine_GL_Context    Render_Engine_GL_Context;

struct _Render_Engine_GL_Surface
{
   int     initialized;
   int     w, h;

#ifdef EVAS_GL
   GLenum  internal_fmt;
#endif
   int     internal_cpp;   // Component per pixel.  ie. RGB = 3

   int     depth_bits;
   int     stencil_bits;

   // Data
   void   *buffer;

   Render_Engine_GL_Context   *current_ctx;
};

#ifdef EVAS_GL
struct _Render_Engine_GL_Context
{
   int            initialized;

   OSMesaContext  context;

   Render_Engine_GL_Context   *share_ctx;

   Render_Engine_GL_Surface   *current_sfc;
};

//------------------------------------------------------//
typedef void                   (*_eng_fn) (void );
typedef _eng_fn                (*glsym_func_eng_fn) ();
typedef void                   (*glsym_func_void) ();
typedef unsigned int           (*glsym_func_uint) ();
typedef int                    (*glsym_func_int) ();
typedef unsigned char          (*glsym_func_uchar) ();
typedef unsigned char         *(*glsym_func_uchar_ptr) ();
typedef const unsigned char   *(*glsym_func_const_uchar_ptr) ();
typedef char const            *(*glsym_func_char_const_ptr) ();
typedef GLboolean              (*glsym_func_bool) ();
typedef OSMesaContext          (*glsym_func_osm_ctx) ();
//------------------------------------------------------//

/* Function table for GL APIs */
static Evas_GL_API gl_funcs;
static void *gl_lib_handle;
static int gl_lib_is_gles = 0;

static Eina_Bool _tls_init = EINA_FALSE;
static Eina_TLS gl_current_ctx_key = 0;
static Eina_TLS gl_current_sfc_key = 0;

//------------------------------------------------------//
// OSMesa APIS...
static OSMesaContext (*_sym_OSMesaCreateContextExt)             (GLenum format, GLint depthBits, GLint stencilBits, GLint accumBits, OSMesaContext sharelist) = NULL;
static void          (*_sym_OSMesaDestroyContext)               (OSMesaContext ctx) = NULL;
static GLboolean     (*_sym_OSMesaMakeCurrent)                  (OSMesaContext ctx, void *buffer, GLenum type, GLsizei width, GLsizei height) = NULL;
static void          (*_sym_OSMesaPixelStore)                   (GLint pname, GLint value) = NULL;
static OSMESAproc    (*_sym_OSMesaGetProcAddress)               (const char *funcName);


//------------------------------------------------------//
// GLES 2.0 APIs...
static void       (*_sym_glActiveTexture)                       (GLenum texture) = NULL;
static void       (*_sym_glAttachShader)                        (GLuint program, GLuint shader) = NULL;
static void       (*_sym_glBindAttribLocation)                  (GLuint program, GLuint index, const char* name) = NULL;
static void       (*_sym_glBindBuffer)                          (GLenum target, GLuint buffer) = NULL;
static void       (*_sym_glBindFramebuffer)                     (GLenum target, GLuint framebuffer) = NULL;
static void       (*_sym_glBindRenderbuffer)                    (GLenum target, GLuint renderbuffer) = NULL;
static void       (*_sym_glBindTexture)                         (GLenum target, GLuint texture) = NULL;
static void       (*_sym_glBlendColor)                          (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = NULL;
static void       (*_sym_glBlendEquation)                       (GLenum mode) = NULL;
static void       (*_sym_glBlendEquationSeparate)               (GLenum modeRGB, GLenum modeAlpha) = NULL;
static void       (*_sym_glBlendFunc)                           (GLenum sfactor, GLenum dfactor) = NULL;
static void       (*_sym_glBlendFuncSeparate)                   (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) = NULL;
static void       (*_sym_glBufferData)                          (GLenum target, GLsizeiptr size, const void* data, GLenum usage) = NULL;
static void       (*_sym_glBufferSubData)                       (GLenum target, GLintptr offset, GLsizeiptr size, const void* data) = NULL;
static GLenum     (*_sym_glCheckFramebufferStatus)              (GLenum target) = NULL;
static void       (*_sym_glClear)                               (GLbitfield mask) = NULL;
static void       (*_sym_glClearColor)                          (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = NULL;
static void       (*_sym_glClearDepthf)                         (GLclampf depth) = NULL;
static void       (*_sym_glClearStencil)                        (GLint s) = NULL;
static void       (*_sym_glColorMask)                           (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = NULL;
static void       (*_sym_glCompileShader)                       (GLuint shader) = NULL;
static void       (*_sym_glCompressedTexImage2D)                (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) = NULL;
static void       (*_sym_glCompressedTexSubImage2D)             (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) = NULL;
static void       (*_sym_glCopyTexImage2D)                      (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) = NULL;
static void       (*_sym_glCopyTexSubImage2D)                   (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static GLuint     (*_sym_glCreateProgram)                       (void) = NULL;
static GLuint     (*_sym_glCreateShader)                        (GLenum type) = NULL;
static void       (*_sym_glCullFace)                            (GLenum mode) = NULL;
static void       (*_sym_glDeleteBuffers)                       (GLsizei n, const GLuint* buffers) = NULL;
static void       (*_sym_glDeleteFramebuffers)                  (GLsizei n, const GLuint* framebuffers) = NULL;
static void       (*_sym_glDeleteProgram)                       (GLuint program) = NULL;
static void       (*_sym_glDeleteRenderbuffers)                 (GLsizei n, const GLuint* renderbuffers) = NULL;
static void       (*_sym_glDeleteShader)                        (GLuint shader) = NULL;
static void       (*_sym_glDeleteTextures)                      (GLsizei n, const GLuint* textures) = NULL;
static void       (*_sym_glDepthFunc)                           (GLenum func) = NULL;
static void       (*_sym_glDepthMask)                           (GLboolean flag) = NULL;
static void       (*_sym_glDepthRangef)                         (GLclampf zNear, GLclampf zFar) = NULL;
static void       (*_sym_glDetachShader)                        (GLuint program, GLuint shader) = NULL;
static void       (*_sym_glDisable)                             (GLenum cap) = NULL;
static void       (*_sym_glDisableVertexAttribArray)            (GLuint index) = NULL;
static void       (*_sym_glDrawArrays)                          (GLenum mode, GLint first, GLsizei count) = NULL;
static void       (*_sym_glDrawElements)                        (GLenum mode, GLsizei count, GLenum type, const void* indices) = NULL;
static void       (*_sym_glEnable)                              (GLenum cap) = NULL;
static void       (*_sym_glEnableVertexAttribArray)             (GLuint index) = NULL;
static void       (*_sym_glFinish)                              (void) = NULL;
static void       (*_sym_glFlush)                               (void) = NULL;
static void       (*_sym_glFramebufferRenderbuffer)             (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = NULL;
static void       (*_sym_glFramebufferTexture2D)                (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
static void       (*_sym_glFrontFace)                           (GLenum mode) = NULL;
static void       (*_sym_glGenBuffers)                          (GLsizei n, GLuint* buffers) = NULL;
static void       (*_sym_glGenerateMipmap)                      (GLenum target) = NULL;
static void       (*_sym_glGenFramebuffers)                     (GLsizei n, GLuint* framebuffers) = NULL;
static void       (*_sym_glGenRenderbuffers)                    (GLsizei n, GLuint* renderbuffers) = NULL;
static void       (*_sym_glGenTextures)                         (GLsizei n, GLuint* textures) = NULL;
static void       (*_sym_glGetActiveAttrib)                     (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name) = NULL;
static void       (*_sym_glGetActiveUniform)                    (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name) = NULL;
static void       (*_sym_glGetAttachedShaders)                  (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) = NULL;
static int        (*_sym_glGetAttribLocation)                   (GLuint program, const char* name) = NULL;
static void       (*_sym_glGetBooleanv)                         (GLenum pname, GLboolean* params) = NULL;
static void       (*_sym_glGetBufferParameteriv)                (GLenum target, GLenum pname, GLint* params) = NULL;
static GLenum     (*_sym_glGetError)                            (void) = NULL;
static void       (*_sym_glGetFloatv)                           (GLenum pname, GLfloat* params) = NULL;
static void       (*_sym_glGetFramebufferAttachmentParameteriv) (GLenum target, GLenum attachment, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetIntegerv)                         (GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetProgramiv)                        (GLuint program, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetProgramInfoLog)                   (GLuint program, GLsizei bufsize, GLsizei* length, char* infolog) = NULL;
static void       (*_sym_glGetRenderbufferParameteriv)          (GLenum target, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetShaderiv)                         (GLuint shader, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetShaderInfoLog)                    (GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog) = NULL;
static void       (*_sym_glGetShaderPrecisionFormat)            (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) = NULL;
static void       (*_sym_glGetShaderSource)                     (GLuint shader, GLsizei bufsize, GLsizei* length, char* source) = NULL;
static const GLubyte *(*_sym_glGetString)                       (GLenum name) = NULL;
static void       (*_sym_glGetTexParameterfv)                   (GLenum target, GLenum pname, GLfloat* params) = NULL;
static void       (*_sym_glGetTexParameteriv)                   (GLenum target, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetUniformfv)                        (GLuint program, GLint location, GLfloat* params) = NULL;
static void       (*_sym_glGetUniformiv)                        (GLuint program, GLint location, GLint* params) = NULL;
static int        (*_sym_glGetUniformLocation)                  (GLuint program, const char* name) = NULL;
static void       (*_sym_glGetVertexAttribfv)                   (GLuint index, GLenum pname, GLfloat* params) = NULL;
static void       (*_sym_glGetVertexAttribiv)                   (GLuint index, GLenum pname, GLint* params) = NULL;
static void       (*_sym_glGetVertexAttribPointerv)             (GLuint index, GLenum pname, void** pointer) = NULL;
static void       (*_sym_glHint)                                (GLenum target, GLenum mode) = NULL;
static GLboolean  (*_sym_glIsBuffer)                            (GLuint buffer) = NULL;
static GLboolean  (*_sym_glIsEnabled)                           (GLenum cap) = NULL;
static GLboolean  (*_sym_glIsFramebuffer)                       (GLuint framebuffer) = NULL;
static GLboolean  (*_sym_glIsProgram)                           (GLuint program) = NULL;
static GLboolean  (*_sym_glIsRenderbuffer)                      (GLuint renderbuffer) = NULL;
static GLboolean  (*_sym_glIsShader)                            (GLuint shader) = NULL;
static GLboolean  (*_sym_glIsTexture)                           (GLuint texture) = NULL;
static void       (*_sym_glLineWidth)                           (GLfloat width) = NULL;
static void       (*_sym_glLinkProgram)                         (GLuint program) = NULL;
static void       (*_sym_glPixelStorei)                         (GLenum pname, GLint param) = NULL;
static void       (*_sym_glPolygonOffset)                       (GLfloat factor, GLfloat units) = NULL;
static void       (*_sym_glReadPixels)                          (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) = NULL;
static void       (*_sym_glReleaseShaderCompiler)               (void) = NULL;
static void       (*_sym_glRenderbufferStorage)                 (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
static void       (*_sym_glSampleCoverage)                      (GLclampf value, GLboolean invert) = NULL;
static void       (*_sym_glScissor)                             (GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
static void       (*_sym_glShaderBinary)                        (GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length) = NULL;
static void       (*_sym_glShaderSource)                        (GLuint shader, GLsizei count, const char* const* string, const GLint* length) = NULL;
static void       (*_sym_glStencilFunc)                         (GLenum func, GLint ref, GLuint mask) = NULL;
static void       (*_sym_glStencilFuncSeparate)                 (GLenum face, GLenum func, GLint ref, GLuint mask) = NULL;
static void       (*_sym_glStencilMask)                         (GLuint mask) = NULL;
static void       (*_sym_glStencilMaskSeparate)                 (GLenum face, GLuint mask) = NULL;
static void       (*_sym_glStencilOp)                           (GLenum fail, GLenum zfail, GLenum zpass) = NULL;
static void       (*_sym_glStencilOpSeparate)                   (GLenum face, GLenum fail, GLenum zfail, GLenum zpass) = NULL;
static void       (*_sym_glTexImage2D)                          (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) = NULL;
static void       (*_sym_glTexParameterf)                       (GLenum target, GLenum pname, GLfloat param) = NULL;
static void       (*_sym_glTexParameterfv)                      (GLenum target, GLenum pname, const GLfloat* params) = NULL;
static void       (*_sym_glTexParameteri)                       (GLenum target, GLenum pname, GLint param) = NULL;
static void       (*_sym_glTexParameteriv)                      (GLenum target, GLenum pname, const GLint* params) = NULL;
static void       (*_sym_glTexSubImage2D)                       (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) = NULL;
static void       (*_sym_glUniform1f)                           (GLint location, GLfloat x) = NULL;
static void       (*_sym_glUniform1fv)                          (GLint location, GLsizei count, const GLfloat* v) = NULL;
static void       (*_sym_glUniform1i)                           (GLint location, GLint x) = NULL;
static void       (*_sym_glUniform1iv)                          (GLint location, GLsizei count, const GLint* v) = NULL;
static void       (*_sym_glUniform2f)                           (GLint location, GLfloat x, GLfloat y) = NULL;
static void       (*_sym_glUniform2fv)                          (GLint location, GLsizei count, const GLfloat* v) = NULL;
static void       (*_sym_glUniform2i)                           (GLint location, GLint x, GLint y) = NULL;
static void       (*_sym_glUniform2iv)                          (GLint location, GLsizei count, const GLint* v) = NULL;
static void       (*_sym_glUniform3f)                           (GLint location, GLfloat x, GLfloat y, GLfloat z) = NULL;
static void       (*_sym_glUniform3fv)                          (GLint location, GLsizei count, const GLfloat* v) = NULL;
static void       (*_sym_glUniform3i)                           (GLint location, GLint x, GLint y, GLint z) = NULL;
static void       (*_sym_glUniform3iv)                          (GLint location, GLsizei count, const GLint* v) = NULL;
static void       (*_sym_glUniform4f)                           (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
static void       (*_sym_glUniform4fv)                          (GLint location, GLsizei count, const GLfloat* v) = NULL;
static void       (*_sym_glUniform4i)                           (GLint location, GLint x, GLint y, GLint z, GLint w) = NULL;
static void       (*_sym_glUniform4iv)                          (GLint location, GLsizei count, const GLint* v) = NULL;
static void       (*_sym_glUniformMatrix2fv)                    (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void       (*_sym_glUniformMatrix3fv)                    (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void       (*_sym_glUniformMatrix4fv)                    (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) = NULL;
static void       (*_sym_glUseProgram)                          (GLuint program) = NULL;
static void       (*_sym_glValidateProgram)                     (GLuint program) = NULL;
static void       (*_sym_glVertexAttrib1f)                      (GLuint indx, GLfloat x) = NULL;
static void       (*_sym_glVertexAttrib1fv)                     (GLuint indx, const GLfloat* values) = NULL;
static void       (*_sym_glVertexAttrib2f)                      (GLuint indx, GLfloat x, GLfloat y) = NULL;
static void       (*_sym_glVertexAttrib2fv)                     (GLuint indx, const GLfloat* values) = NULL;
static void       (*_sym_glVertexAttrib3f)                      (GLuint indx, GLfloat x, GLfloat y, GLfloat z) = NULL;
static void       (*_sym_glVertexAttrib3fv)                     (GLuint indx, const GLfloat* values) = NULL;
static void       (*_sym_glVertexAttrib4f)                      (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
static void       (*_sym_glVertexAttrib4fv)                     (GLuint indx, const GLfloat* values) = NULL;
static void       (*_sym_glVertexAttribPointer)                 (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr) = NULL;
static void       (*_sym_glViewport)                            (GLint x, GLint y, GLsizei width, GLsizei height) = NULL;

// GLES Extensions...
/* static void       (*_sym_glGetProgramBinary)                    (GLuint a, GLsizei b, GLsizei* c, GLenum* d, void* e) = NULL; */
/* static void       (*_sym_glProgramBinary)                       (GLuint a, GLenum b, const void* c, GLint d) = NULL; */
/* static void       (*_sym_glProgramParameteri)                   (GLuint a, GLuint b, GLint d) = NULL; */

static int gl_lib_init(void);
#endif

// Threaded Render

/**
 * @brief Structure holding data for a rectangle drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Rect Evas_Thread_Command_Rect;

/**
 * @brief Structure holding data for a line drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Line Evas_Thread_Command_Line;
/**
 * @brief Structure holding data for a polygon drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Polygon Evas_Thread_Command_Polygon;

/**
 * @brief Structure holding data for an image drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Image Evas_Thread_Command_Image;

/**
 * @brief Structure holding data for a font (glyph array) drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Font Evas_Thread_Command_Font;
/**
 * @brief Structure holding data for a map drawing command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Map Evas_Thread_Command_Map;

/**
 * @brief Structure holding data for drawing multiple font runs in a separate thread.
 */
typedef struct _Evas_Thread_Command_Multi_Font Evas_Thread_Command_Multi_Font;

/**
 * @brief Structure holding data for an Ector rendering command in a separate thread.
 */
typedef struct _Evas_Thread_Command_Ector Evas_Thread_Command_Ector;

/**
 * @brief Structure holding data for setting up an Ector surface in a separate thread.
 */
typedef struct _Evas_Thread_Command_Ector_Surface Evas_Thread_Command_Ector_Surface;

/**
 * @brief Holds parameters for drawing a rectangle in a rendering thread.
 */
struct _Evas_Thread_Command_Rect
{
   void *surface;
   DATA32 color;
   int render_op;
   int x, y, w, h;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
};

/**
 * @brief Holds parameters for drawing a line in a rendering thread.
 */
struct _Evas_Thread_Command_Line
{
   void *surface; /**< Target surface (RGBA_Image). */
   Eina_Rectangle clip;
   DATA32 color;
   int render_op;
   Eina_Bool anti_alias;
   int x1, y1;
   int x2, y2;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
};

/**
 * @brief Holds parameters for drawing a polygon in a rendering thread.
 */
struct _Evas_Thread_Command_Polygon
{
   Eina_Rectangle ext; /**< Clipping extent rectangle. */
   DATA32 col;
   int render_op;
   void *surface;
   RGBA_Polygon_Point *points;
   int x, y;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
};

/**
 * @brief Holds parameters for drawing an image in a rendering thread.
 */
struct _Evas_Thread_Command_Image
{
   void *surface; /**< Target surface (RGBA_Image). */
   void *image;
   Eina_Rectangle src, dst, clip;
   DATA32 mul_col;
   int render_op;
   int smooth;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
};

/**
 * @brief Holds parameters for drawing text (glyphs) in a rendering thread.
 */
struct _Evas_Thread_Command_Font
{
   RGBA_Image *dst; /**< Destination surface. */
   int x;
   int y;
   Evas_Glyph_Array *glyphs;
   RGBA_Gfx_Func func;
   void *gl_new;
   void *gl_free;
   void *gl_draw;
   void *font_ext_data;
   DATA32 col;
   DATA32 mul_col;
   Eina_Rectangle clip_rect, ext;
   int im_w, im_h;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
   Eina_Bool clip_use : 1; /**< Flag indicating if clipping is used. */
};

/**
 * @brief Holds parameters for drawing a mapped image in a rendering thread.
 */
struct _Evas_Thread_Command_Map
{
   void *image; /**< Source image (RGBA_Image). */
   RGBA_Draw_Context *image_ctx;
   void *surface;
   Eina_Rectangle clip;
   DATA32 mul_col;
   int render_op;
   RGBA_Map *map;
   int smooth, level, offset;
   Eina_Bool anti_alias;
   void *mask;
   int mask_x; /**< X offset for the mask surface. */
   int mask_y; /**< Y offset for the mask surface. */
};

/**
 * @brief Holds parameters for drawing multiple text runs in a rendering thread.
 */
struct _Evas_Thread_Command_Multi_Font
{
   RGBA_Draw_Context *context; /**< Drawing context (duplicated for the thread). */
   void *surface;
   int x, y;
   Evas_Font_Array *texts; /**< Array of text properties and glyphs to draw. */
};

/**
 * @brief Holds parameters for executing an Ector renderer draw command in a thread.
 */
struct _Evas_Thread_Command_Ector
{
   Ector_Renderer *r; /**< The Ector renderer to use. */
   Eina_Array *clips;

   DATA32 mul_col;
   Efl_Gfx_Render_Op render_op;

   Eina_Bool free_it; /**< Flag indicating if this command struct should be freed after execution. */
};

/**
 * @brief Holds parameters for setting up the target surface for an Ector renderer in a thread.
 */
struct _Evas_Thread_Command_Ector_Surface
{
   Ector_Surface *ector; /**< The Ector surface being configured. */
   void *pixels;
   int x; /**< X reference point for the surface. */
   int y; /**< Y reference point for the surface. */
};

// declare here as it is re-used
/**
 * @brief Creates a new surface suitable for image map operations.
 *
 * This function allocates an RGBA_Image that can be used as a temporary
 * drawing surface, typically for map rendering operations where an
 * intermediate buffer is needed.
 *
 * @param data Engine-specific data (unused).
 * @param w Width of the surface.
 * @param h Height of the surface.
 * @param alpha Boolean indicating if the surface should support alpha.
 * @return A pointer to the newly created RGBA_Image surface, or NULL on failure.
 */
static void *eng_image_map_surface_new(void *data, int w, int h, int alpha);

Eina_Mempool *_mp_command_rect = NULL;
Eina_Mempool *_mp_command_line = NULL;
Eina_Mempool *_mp_command_polygon = NULL;
Eina_Mempool *_mp_command_image = NULL;
Eina_Mempool *_mp_command_font = NULL;
Eina_Mempool *_mp_command_map = NULL;
Eina_Mempool *_mp_command_multi_font = NULL;
Eina_Mempool *_mp_command_ector = NULL;
Eina_Mempool *_mp_command_ector_surface = NULL;
/*
 *****
 **
 ** ENGINE ROUTINES
 **
 *****
 */
static int cpunum = 0; /**< Number of available CPU cores, used for potential optimizations. */
static int _evas_soft_gen_log_dom = -1; /**< Log domain for the software generic engine. */

//#define QCMD evas_thread_cmd_enqueue // Alternative command queuing macro (unused)
#define QCMD evas_thread_queue_flush /**< Macro used to flush the command queue for threaded rendering. */

/**
 * @brief Dumps caches (image and font). Called usually before shutdown or on memory pressure.
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 */
static void
eng_output_dump(void *engine EINA_UNUSED, void *data EINA_UNUSED)
{
   evas_common_image_image_all_unload();
   evas_common_font_font_all_unload();
}

/**
 * @brief Creates a new drawing context.
 * @param data Engine-specific data (unused).
 * @return A new RGBA_Draw_Context instance.
 */
static void *
eng_context_new(void *data EINA_UNUSED)
{
   return evas_common_draw_context_new();
}

/**
 * @brief Duplicates an existing drawing context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to duplicate.
 * @return A new RGBA_Draw_Context instance, which is a copy of the input context.
 *         References to mask images are incremented.
 */
static void *
eng_context_dup(void *data EINA_UNUSED, void *context)
{
   RGBA_Draw_Context *ctx;

   ctx = evas_common_draw_context_dup(context);
   if (ctx->clip.mask)
     {
        Image_Entry *im = ctx->clip.mask;
        evas_cache_image_ref(im);
     }

   return ctx;
}

/**
 * @brief Sets the rectangular clipping region for a drawing context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param x The x-coordinate of the clip rectangle.
 * @param y The y-coordinate of the clip rectangle.
 * @param w The width of the clip rectangle.
 * @param h The height of the clip rectangle.
 */
static void
eng_context_clip_set(void *data EINA_UNUSED, void *context, int x, int y, int w, int h)
{
   evas_common_draw_context_set_clip(context, x, y, w, h);
}

/**
 * @brief Unsets the image mask clipping for a drawing context.
 * Decrements the reference count of the mask image if set.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 */
static void
eng_context_clip_image_unset(void *data EINA_UNUSED, void *context)
{
   RGBA_Draw_Context *ctx = context;

   if (ctx->clip.mask)
     {
        Image_Entry *ie = ctx->clip.mask;

        if (ctx->clip.async)
          evas_unref_queue_image_put(ctx->clip.evas, ie);
        else
          evas_cache_image_drop(ie);
        ctx->clip.mask = NULL;
     }
}

/**
 * @brief Sets an image mask for clipping in a drawing context.
 * The drawing operations will be masked by the alpha channel of the provided image.
 * The rectangular clip region is intersected with the mask image bounds.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param surface The mask image (Image_Entry). Reference count is incremented.
 * @param x The x-coordinate offset for the mask image.
 * @param y The y-coordinate offset for the mask image.
 * @param evas The Evas canvas public data (used for async unref).
 * @param do_async If true, use async unref queue for the mask image.
 */
static void
eng_context_clip_image_set(void *data EINA_UNUSED, void *context, void *surface, int x, int y,
                           Evas_Public_Data *evas, Eina_Bool do_async)
{
   RGBA_Draw_Context *ctx = context;
   Eina_Bool noinc = EINA_FALSE;

   if (ctx->clip.mask)
     {
        if (ctx->clip.mask != surface)
          eng_context_clip_image_unset(data, context);
        else
          noinc = EINA_TRUE;
     }

   ctx->clip.mask = surface;
   ctx->clip.mask_x = x;
   ctx->clip.mask_y = y;
   ctx->clip.evas = evas;
   ctx->clip.async = do_async;

   if (surface)
     {
        Image_Entry *ie = surface;
        if (!noinc)
          evas_cache_image_ref(ie);
        RECTS_CLIP_TO_RECT(ctx->clip.x, ctx->clip.y, ctx->clip.w, ctx->clip.h,
                           x, y, ie->w, ie->h);
     }
}

/**
 * @brief Gets the current image mask used for clipping.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @param ie Pointer to store the mask image (Image_Entry). Reference count is incremented if not NULL.
 * @param x Pointer to store the x-coordinate offset of the mask.
 * @param y Pointer to store the y-coordinate offset of the mask.
 */
static void
eng_context_clip_image_get(void *data EINA_UNUSED, void *context, void **ie, int *x, int *y)
{
   RGBA_Draw_Context *ctx = context;

   if (ie)
     {
        Image_Entry *im = ctx->clip.mask;

        *ie = im;
        if (im)
          evas_cache_image_ref(im);
     }
   if (x) *x = ctx->clip.mask_x;
   if (y) *y = ctx->clip.mask_y;
}

/**
 * @brief Frees a drawing context.
 * Unsets any image mask before freeing.
 * @param data Engine-specific data (used by eng_context_clip_image_unset).
 * @param context The RGBA_Draw_Context to free.
 */
static void
eng_context_free(void *data, void *context)
{
   RGBA_Draw_Context *ctx = context;

   if (!ctx) return;
   if (ctx->clip.mask)
     eng_context_clip_image_unset(data, context);
   evas_common_draw_context_free(context);
}

/**
 * @brief Intersects the current clip rectangle with a new rectangle.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param x The x-coordinate of the new rectangle.
 * @param y The y-coordinate of the new rectangle.
 * @param w The width of the new rectangle.
 * @param h The height of the new rectangle.
 */
static void
eng_context_clip_clip(void *data EINA_UNUSED, void *context, int x, int y, int w, int h)
{
   evas_common_draw_context_clip_clip(context, x, y, w, h);
}

/**
 * @brief Resets the clipping region to the default (no clipping).
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 */
static void
eng_context_clip_unset(void *data EINA_UNUSED, void *context)
{
   evas_common_draw_context_unset_clip(context);
}

/**
 * @brief Gets the current rectangular clipping region.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @param x Pointer to store the x-coordinate of the clip rectangle.
 * @param y Pointer to store the y-coordinate of the clip rectangle.
 * @param w Pointer to store the width of the clip rectangle.
 * @param h Pointer to store the height of the clip rectangle.
 * @return 1 if clipping is enabled, 0 otherwise.
 */
static int
eng_context_clip_get(void *data EINA_UNUSED, void *context, int *x, int *y, int *w, int *h)
{
   if (x) *x = ((RGBA_Draw_Context *)context)->clip.x;
   if (y) *y = ((RGBA_Draw_Context *)context)->clip.y;
   if (w) *w = ((RGBA_Draw_Context *)context)->clip.w;
   if (h) *h = ((RGBA_Draw_Context *)context)->clip.h;
   return ((RGBA_Draw_Context *)context)->clip.use;
}

/**
 * @brief Sets the drawing color for a context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
static void
eng_context_color_set(void *data EINA_UNUSED, void *context, int r, int g, int b, int a)
{
   evas_common_draw_context_set_color(context, r, g, b, a);
}

/**
 * @brief Gets the current drawing color from a context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @param r Pointer to store the red component.
 * @param g Pointer to store the green component.
 * @param b Pointer to store the blue component.
 * @param a Pointer to store the alpha component.
 * @return Always returns 1 (success).
 */
static int
eng_context_color_get(void *data EINA_UNUSED, void *context, int *r, int *g, int *b, int *a)
{
   *r = (int)(R_VAL(&((RGBA_Draw_Context *)context)->col.col));
   *g = (int)(G_VAL(&((RGBA_Draw_Context *)context)->col.col));
   *b = (int)(B_VAL(&((RGBA_Draw_Context *)context)->col.col));
   *a = (int)(A_VAL(&((RGBA_Draw_Context *)context)->col.col));
   return 1;
}

/**
 * @brief Sets the color multiplier for a context.
 * The drawing color and source image colors will be multiplied by this color.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param r Red multiplier component (0-255).
 * @param g Green multiplier component (0-255).
 * @param b Blue multiplier component (0-255).
 * @param a Alpha multiplier component (0-255).
 */
static void
eng_context_multiplier_set(void *data EINA_UNUSED, void *context, int r, int g, int b, int a)
{
   evas_common_draw_context_set_multiplier(context, r, g, b, a);
}

/**
 * @brief Unsets the color multiplier (disables multiplication).
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 */
static void
eng_context_multiplier_unset(void *data EINA_UNUSED, void *context)
{
   evas_common_draw_context_unset_multiplier(context);
}

/**
 * @brief Gets the current color multiplier from a context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @param r Pointer to store the red multiplier component.
 * @param g Pointer to store the green multiplier component.
 * @param b Pointer to store the blue multiplier component.
 * @param a Pointer to store the alpha multiplier component.
 * @return 1 if a multiplier is set, 0 otherwise.
 */
static int
eng_context_multiplier_get(void *data EINA_UNUSED, void *context, int *r, int *g, int *b, int *a)
{
   *r = (int)(R_VAL(&((RGBA_Draw_Context *)context)->mul.col));
   *g = (int)(G_VAL(&((RGBA_Draw_Context *)context)->mul.col));
   *b = (int)(B_VAL(&((RGBA_Draw_Context *)context)->mul.col));
   *a = (int)(A_VAL(&((RGBA_Draw_Context *)context)->mul.col));
   return ((RGBA_Draw_Context *)context)->mul.use;
}

/**
 * @brief Adds a rectangular cutout region to the context.
 * Drawing operations will not affect pixels within cutout regions.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param x The x-coordinate of the cutout rectangle.
 * @param y The y-coordinate of the cutout rectangle.
 * @param w The width of the cutout rectangle.
 * @param h The height of the cutout rectangle.
 */
static void
eng_context_cutout_add(void *data EINA_UNUSED, void *context, int x, int y, int w, int h)
{
   evas_common_draw_context_add_cutout(context, x, y, w, h);
}

/**
 * @brief Clears all cutout regions from the context.
 * Also resets the cutout target rectangle.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 */
static void
eng_context_cutout_clear(void *data EINA_UNUSED, void *context)
{
   evas_common_draw_context_target_set(context, 0, 0, 0, 0);
   evas_common_draw_context_clear_cutouts(context);
}

/**
 * @brief Sets the target area for subsequent cutout additions.
 * Cutouts added after this call will be relative to and clipped by this target area.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param x The x-coordinate of the target rectangle.
 * @param y The y-coordinate of the target rectangle.
 * @param w The width of the target rectangle.
 * @param h The height of the target rectangle.
 */
static void
eng_context_cutout_target(void *data EINA_UNUSED, void *context, int x, int y, int w, int h)
{
   evas_common_draw_context_target_set(context, x, y, w, h);
}

/**
 * @brief Sets the anti-aliasing mode for the context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param aa 1 to enable anti-aliasing, 0 to disable.
 */
static void
eng_context_anti_alias_set(void *data EINA_UNUSED, void *context, unsigned char aa)
{
   evas_common_draw_context_set_anti_alias(context, aa);
}

/**
 * @brief Gets the current anti-aliasing mode from the context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @return 1 if anti-aliasing is enabled, 0 otherwise.
 */
static unsigned char
eng_context_anti_alias_get(void *data EINA_UNUSED, void *context)
{
   return ((RGBA_Draw_Context *)context)->anti_alias;
}

/**
 * @brief Sets the color interpolation space for the context.
 * Affects how colors are interpolated, e.g., during gradient rendering (if applicable).
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param color_space The Evas_Colorspace value for interpolation.
 */
static void
eng_context_color_interpolation_set(void *data EINA_UNUSED, void *context, int color_space)
{
   evas_common_draw_context_set_color_interpolation(context, color_space);
}

/**
 * @brief Gets the current color interpolation space from the context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @return The Evas_Colorspace value used for interpolation.
 */
static int
eng_context_color_interpolation_get(void *data EINA_UNUSED, void *context)
{
   return ((RGBA_Draw_Context *)context)->interpolation.color_space;
}

/**
 * @brief Sets the rendering operation (blending mode) for the context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to modify.
 * @param op The Evas_Render_Op value (e.g., EVAS_RENDER_BLEND, EVAS_RENDER_COPY).
 */
static void
eng_context_render_op_set(void *data EINA_UNUSED, void *context, int op)
{
   evas_common_draw_context_set_render_op(context, op);
}

/**
 * @brief Gets the current rendering operation (blending mode) from the context.
 * @param data Engine-specific data (unused).
 * @param context The RGBA_Draw_Context to query.
 * @return The Evas_Render_Op value.
 */
static int
eng_context_render_op_get(void *data EINA_UNUSED, void *context)
{
   return ((RGBA_Draw_Context *)context)->render_op;
}

/**
 * @brief Executes a rectangle drawing command in a rendering thread.
 * This function is called by the thread pool.
 * @param data Pointer to an Evas_Thread_Command_Rect structure containing drawing parameters.
 */
static void
_draw_thread_rectangle_draw(void *data)
{
    Evas_Thread_Command_Rect *rect = data;

    evas_common_rectangle_rgba_draw(rect->surface,
                                    rect->color, rect->render_op,
                                    rect->x, rect->y, rect->w, rect->h,
                                    rect->mask, rect->mask_x, rect->mask_y);

    eina_mempool_free(_mp_command_rect, rect);
}

/**
 * @brief Creates and enqueues a rectangle drawing command for threaded execution.
 * @param dst The destination RGBA_Image surface.
 * @param dc The drawing context.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
static void
_draw_rectangle_thread_cmd(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   Evas_Thread_Command_Rect *cr;

   RECTS_CLIP_TO_RECT(x, y, w, h, dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
   if ((w <= 0) || (h <= 0)) return;

   cr = eina_mempool_malloc(_mp_command_rect, sizeof (Evas_Thread_Command_Rect));
   if (!cr) return;

   cr->surface = dst;
   cr->color = dc->col.col;
   cr->render_op = dc->render_op;
   cr->x = x;
   cr->y = y;
   cr->w = w;
   cr->h = h;
   cr->mask = dc->clip.mask;
   cr->mask_x = dc->clip.mask_x;
   cr->mask_y = dc->clip.mask_y;

   QCMD(_draw_thread_rectangle_draw, cr);
}

/**
 * @brief Engine function to draw a rectangle.
 * Dispatches the drawing to the appropriate implementation (sync, async thread, pipe).
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 */
static void
eng_rectangle_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, int x, int y, int w, int h, Eina_Bool do_async)
{
   if (do_async)
     evas_common_rectangle_draw_cb(surface, context, x, y, w, h,
                                   _draw_rectangle_thread_cmd);
#ifdef BUILD_PIPE_RENDER
   else if ((cpunum > 1))
     evas_common_pipe_rectangle_draw(surface, context, x, y, w, h);
#endif
   else
     {
        evas_common_rectangle_draw(surface, context, x, y, w, h);
        evas_common_cpu_end_opt();
     }
}

/**
 * @brief Executes a line drawing command in a rendering thread.
 * Handles both single points and lines (aliased or anti-aliased).
 * This function is called by the thread pool.
 * @param data Pointer to an Evas_Thread_Command_Line structure containing drawing parameters.
 */
static void
_draw_thread_line_draw(void *data)
{
   Evas_Thread_Command_Line *line = data;
   int clip_x, clip_y, clip_w, clip_h;

   clip_x = line->clip.x;
   clip_y = line->clip.y;
   clip_w = line->clip.w;
   clip_h = line->clip.h;

   if ((line->x1 == line->x2) && (line->y1 == line->y2))
     {
        evas_common_line_point_draw(line->surface,
                                    clip_x, clip_y, clip_w, clip_h,
                                    line->color, line->render_op,
                                    line->x1, line->y1,
                                    line->mask, line->mask_x, line->mask_y);
        return;
     }

   if (line->anti_alias)
     evas_common_line_draw_line_aa
       (line->surface,
        clip_x, clip_y, clip_w, clip_h,
        line->color, line->render_op,
        line->x1, line->y1,
        line->x2, line->y2,
        line->mask, line->mask_x, line->mask_y);
   else
     evas_common_line_draw_line
       (line->surface,
        clip_x, clip_y, clip_w, clip_h,
        line->color, line->render_op,
        line->x1, line->y1,
        line->x2, line->y2,
        line->mask, line->mask_x, line->mask_y);

   eina_mempool_free(_mp_command_line, line);
}

/**
 * @brief Creates and enqueues a line drawing command for threaded execution.
 * Performs clipping before creating the command structure.
 * @param dst The destination RGBA_Image surface.
 * @param dc The drawing context.
 * @param x1 The starting x-coordinate of the line.
 * @param y1 The starting y-coordinate of the line.
 * @param x2 The ending x-coordinate of the line.
 * @param y2 The ending y-coordinate of the line.
 */
static void
_line_draw_thread_cmd(RGBA_Image *dst, RGBA_Draw_Context *dc, int x1, int y1, int x2, int y2)
{
   Evas_Thread_Command_Line *cl;
   int clx, cly, clw, clh;
   int cx, cy, cw, ch;
   int x, y, w, h;

   cl = eina_mempool_malloc(_mp_command_line, sizeof (Evas_Thread_Command_Line));
   if (!cl) return;

   cl->surface = dst;

   if ((x1 == x2) && (y1 == y2))
     {
        EINA_RECTANGLE_SET(&cl->clip,
                           dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
        goto done;
     }

   clx = cly = 0;
   clw = dst->cache_entry.w;
   clh = dst->cache_entry.h;

   cx = dc->clip.x;
   cy = dc->clip.y;
   cw = dc->clip.w;
   ch = dc->clip.h;

   if (dc->clip.use)
     {
	RECTS_CLIP_TO_RECT(clx, cly, clw, clh, cx, cy, cw, ch);
	if ((clw < 1) || (clh < 1))
          {
             eina_mempool_free(_mp_command_line, cl);
             return;
          }
     }

   x = MIN(x1, x2);
   y = MIN(y1, y2);
   w = MAX(x1, x2) - x + 1;
   h = MAX(y1, y2) - y + 1;

   RECTS_CLIP_TO_RECT(clx, cly, clw, clh, x, y, w, h);
   if ((clw < 1) || (clh < 1))
     {
        eina_mempool_free(_mp_command_line, cl);
        return;
     }

   EINA_RECTANGLE_SET(&cl->clip, clx, cly, clw, clh);

 done:
   cl->color = dc->col.col;
   cl->render_op = dc->render_op;
   cl->anti_alias = dc->anti_alias;
   cl->x1 = x1;
   cl->y1 = y1;
   cl->x2 = x2;
   cl->y2 = y2;
   cl->mask = dc->clip.mask;
   cl->mask_x = dc->clip.mask_x;
   cl->mask_y = dc->clip.mask_y;

   QCMD(_draw_thread_line_draw, cl);
}

/**
 * @brief Engine function to draw a line.
 * Dispatches the drawing to the appropriate implementation (sync, async thread, pipe).
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param x1 The starting x-coordinate of the line.
 * @param y1 The starting y-coordinate of the line.
 * @param x2 The ending x-coordinate of the line.
 * @param y2 The ending y-coordinate of the line.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 */
static void
eng_line_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, int x1, int y1, int x2, int y2, Eina_Bool do_async)
{
   if (do_async) _line_draw_thread_cmd(surface, context, x1, y1, x2, y2);
#ifdef BUILD_PIPE_RENDER
   else if ((cpunum > 1))
     evas_common_pipe_line_draw(surface, context, x1, y1, x2, y2);
#endif
   else
     {
        evas_common_line_draw(surface, context, x1, y1, x2, y2);
        evas_common_cpu_end_opt();
     }
}

/**
 * @brief Adds a point to a polygon structure.
 * @param data Engine-specific data (unused).
 * @param polygon The polygon structure (RGBA_Polygon_Point list) to modify.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @return The updated polygon structure (potentially the new head of the list).
 */
static void *
eng_polygon_point_add(void *data EINA_UNUSED, void *polygon, int x, int y)
{
   return evas_common_polygon_point_add(polygon, x, y);
}

/**
 * @brief Clears all points from a polygon structure.
 * Frees the memory associated with the points.
 * @param data Engine-specific data (unused).
 * @param polygon The polygon structure (RGBA_Polygon_Point list) to clear.
 * @return Always returns NULL (representing an empty polygon).
 */
static void *
eng_polygon_points_clear(void *data EINA_UNUSED, void *polygon)
{
   return evas_common_polygon_points_clear(polygon);
}

/**
 * @brief Frees the points list associated with a threaded polygon command.
 * This is called after the polygon has been drawn in the thread.
 * @param poly The polygon command structure whose points need freeing.
 */
static void
_draw_thread_polygon_cleanup(Evas_Thread_Command_Polygon *poly)
{
   RGBA_Polygon_Point *points = poly->points;

   while (points)
     {
        RGBA_Polygon_Point *p;

        p = points;
        points =
          (RGBA_Polygon_Point *)eina_inlist_remove(EINA_INLIST_GET(points),
                                                   EINA_INLIST_GET(points));
        free(p);
     }

   poly->points = NULL;
}

/**
 * @brief Executes a polygon drawing command in a rendering thread.
 * This function is called by the thread pool. Cleans up points afterwards.
 * @param data Pointer to an Evas_Thread_Command_Polygon structure containing drawing parameters.
 */
static void
_draw_thread_polygon_draw(void *data)
{
   Evas_Thread_Command_Polygon *poly = data;

   evas_common_polygon_rgba_draw
     (poly->surface,
      poly->ext.x, poly->ext.y, poly->ext.w, poly->ext.h,
      poly->col, poly->render_op,
      poly->points, poly->x, poly->y,
      poly->mask, poly->mask_x, poly->mask_y);

   _draw_thread_polygon_cleanup(poly);
   eina_mempool_free(_mp_command_polygon, poly);
}

/**
 * @brief Duplicates the points list for a threaded polygon command.
 * The original points list belongs to the main thread, so the rendering thread needs its own copy.
 * @param cp The threaded polygon command structure to populate.
 * @param points The original list of polygon points (RGBA_Polygon_Point list).
 */
static void
_polygon_draw_thread_points_populate(Evas_Thread_Command_Polygon *cp, RGBA_Polygon_Point *points)
{
   RGBA_Polygon_Point *cur, *npoints = NULL;

   if (!points) return;

   EINA_INLIST_FOREACH(EINA_INLIST_GET(points), cur)
     {
        RGBA_Polygon_Point *point;

        point = malloc(sizeof *point);
        point->x = cur->x;
        point->y = cur->y;

        npoints =
          (RGBA_Polygon_Point *)eina_inlist_append(EINA_INLIST_GET(npoints),
                                                   EINA_INLIST_GET(point));
     }

   cp->points = npoints;
}

/**
 * @brief Creates and enqueues a polygon drawing command for threaded execution.
 * Performs clipping and duplicates the points list.
 * @param dst The destination RGBA_Image surface.
 * @param dc The drawing context.
 * @param points The list of polygon points (RGBA_Polygon_Point list).
 * @param x The x-offset for drawing the polygon.
 * @param y The y-offset for drawing the polygon.
 */
static void
_polygon_draw_thread_cmd(RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Polygon_Point *points, int x, int y)
{
   int ext_x, ext_y, ext_w, ext_h;
   Evas_Thread_Command_Polygon *cp;

   ext_x = 0;
   ext_y = 0;
   ext_w = dst->cache_entry.w;
   ext_h = dst->cache_entry.h;

   if (dc->clip.use)
     {
	if (dc->clip.x > ext_x)
	  {
	     ext_w += ext_x - dc->clip.x;
	     ext_x = dc->clip.x;
	  }

	if ((ext_x + ext_w) > (dc->clip.x + dc->clip.w))
          ext_w = (dc->clip.x + dc->clip.w) - ext_x;

	if (dc->clip.y > ext_y)
	  {
	     ext_h += ext_y - dc->clip.y;
	     ext_y = dc->clip.y;
	  }

	if ((ext_y + ext_h) > (dc->clip.y + dc->clip.h))
          ext_h = (dc->clip.y + dc->clip.h) - ext_y;
     }

   cp = eina_mempool_malloc(_mp_command_polygon, sizeof (Evas_Thread_Command_Polygon));
   if (!cp) return;

   EINA_RECTANGLE_SET(&cp->ext, ext_x, ext_y, ext_w, ext_h);
   cp->col = dc->col.col;
   cp->render_op = dc->render_op;
   cp->surface = dst;

   _polygon_draw_thread_points_populate(cp, points);

   cp->x = x;
   cp->y = y;

   cp->mask = dc->clip.mask;
   cp->mask_x = dc->clip.mask_x;
   cp->mask_y = dc->clip.mask_y;

   QCMD(_draw_thread_polygon_draw, cp);
}

/**
 * @brief Engine function to draw a polygon.
 * Dispatches the drawing to the appropriate implementation (sync, async thread, pipe).
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param polygon The polygon points list (RGBA_Polygon_Point list).
 * @param x The x-offset for drawing the polygon.
 * @param y The y-offset for drawing the polygon.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 */
static void
eng_polygon_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, void *polygon, int x, int y, Eina_Bool do_async)
{
   if (do_async) _polygon_draw_thread_cmd(surface, context, polygon, x, y);
#ifdef BUILD_PIPE_RENDER
   else if ((cpunum > 1))
     evas_common_pipe_poly_draw(surface, context, polygon, x, y);
#endif
   else
     {
	evas_common_polygon_draw(surface, context, polygon, x, y);
	evas_common_cpu_end_opt();
     }
}

/**
 * @brief Gets the alpha channel flag for an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return 1 if the image has an alpha channel, 0 otherwise.
 */
static int
eng_image_alpha_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return 1;
   im = image;
   switch (im->space)
     {
      case EVAS_COLORSPACE_ARGB8888:
	if (im->flags.alpha) return 1;
      default:
	break;
     }
   return 0;
}

/**
 * @brief Gets the colorspace of an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The Evas_Colorspace of the image. Defaults to EVAS_COLORSPACE_ARGB8888 if image is NULL.
 */
static Evas_Colorspace
eng_image_colorspace_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EVAS_COLORSPACE_ARGB8888;
   im = image;
   return im->space;
}

/**
 * @brief Checks if the image loader supports region loading/decoding.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return EINA_TRUE if region operations are supported by the loader, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_image_can_region_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;
   if (!image) return EINA_FALSE;
   im = image;
   return ((Evas_Image_Load_Func*) im->info.loader)->do_region;
}

/**
 * @brief Sets the alpha channel flag for an image.
 * If the image data is loaded, it ensures the image is not shared (copies if necessary)
 * before modifying the flag. Marks the image colorspace as dirty.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to modify.
 * @param has_alpha 1 to indicate the image has alpha, 0 otherwise.
 * @return The potentially new image entry if a copy was made, otherwise the original image.
 */
static void *
eng_image_alpha_set(void *data EINA_UNUSED, void *image, int has_alpha)
{
   RGBA_Image *im;

   if (!image) return NULL;
   im = image;
   if (im->cache_entry.space != EVAS_COLORSPACE_ARGB8888)
     {
	im->cache_entry.flags.alpha = 0;
	return im;
     }
   if (!im->image.data) evas_cache_image_load_data(&im->cache_entry);
   im = (RGBA_Image *) evas_cache_image_alone(&im->cache_entry);
   im->cache_entry.flags.alpha = has_alpha ? 1 : 0;
   evas_common_image_colorspace_dirty(im);
   return im;
}

/**
 * @brief Gets the original colorspace of the image as reported by the loader.
 * This might differ from the current colorspace if it was converted.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The original Evas_Colorspace from the file, or the current space if unavailable.
 */
static Evas_Colorspace
eng_image_file_colorspace_get(void *data EINA_UNUSED, void *image)
{
   RGBA_Image *im = image;

   if (!im) return EVAS_COLORSPACE_ARGB8888;
   if (im->cache_entry.cspaces)
     return im->cache_entry.cspaces[0];
   return im->cache_entry.space;
}

/**
 * @brief Gets the content (non-border) region of an image, if defined by the loader.
 * Loads image data if necessary.
 * @param engine The engine instance (unused).
 * @param image The image entry (Image_Entry).
 * @param content Pointer to an Eina_Rectangle to store the content region.
 * @return EINA_TRUE if a content region is available and returned, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_image_content_region_get(void *engine EINA_UNUSED, void *image, Eina_Rectangle *content)
{
   RGBA_Image *im = image;

   if (!im) return EINA_FALSE;

   if (!im->cache_entry.need_data) return EINA_FALSE;

   if (!im->image.data) evas_cache_image_load_data(&im->cache_entry);

   if (!im->cache_entry.content.w ||
       !im->cache_entry.content.h)
     return EINA_FALSE;

   if (!content) return EINA_FALSE;

   memcpy(content, &im->cache_entry.content, sizeof (Eina_Rectangle));
   return EINA_TRUE;
}

/**
 * @brief Gets the stretch regions (9-patch data) of an image, if defined by the loader.
 * Loads image data if necessary.
 * @param engine The engine instance (unused).
 * @param image The image entry (Image_Entry).
 * @param horizontal Pointer to store the horizontal stretch region data.
 * @param vertical Pointer to store the vertical stretch region data.
 * @return EINA_TRUE if stretch regions are available and returned, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_image_stretch_region_get(void *engine EINA_UNUSED, void *image,
                             uint8_t **horizontal, uint8_t **vertical)
{
   RGBA_Image *im = image;

   if (!im) return EINA_FALSE;

   if (!im->cache_entry.need_data) return EINA_FALSE;

   if (!im->image.data) evas_cache_image_load_data(&im->cache_entry);

   if (!im->cache_entry.stretch.horizontal.region ||
       !im->cache_entry.stretch.vertical.region)
     return EINA_FALSE;

   *horizontal = im->cache_entry.stretch.horizontal.region;
   *vertical = im->cache_entry.stretch.vertical.region;
   return EINA_TRUE;
}

/**
 * @brief Gets direct access to image data via an Eina_Slice.
 * This provides a view into the image's internal buffer without copying, if possible
 * for the given colorspace and plane.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @param plane The color plane to access (0 for interleaved formats like ARGB,
 *              0=Y, 1=U, 2=V etc. for planar YUV).
 * @param slice Pointer to an Eina_Slice to store the data pointer and length.
 * @param cspace Pointer to store the Evas_Colorspace of the returned data.
 * @param load If EINA_TRUE, ensures image data is loaded before returning the slice.
 * @param tofree Pointer to a boolean that will be set to EINA_TRUE if the caller
 *               needs to free the slice memory (currently always EINA_FALSE).
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid plane, load error).
 */
static Eina_Bool
eng_image_data_direct_get(void *data EINA_UNUSED, void *image, int plane,
                          Eina_Slice *slice, Evas_Colorspace *cspace,
                          Eina_Bool load, Eina_Bool *tofree)
{
   RGBA_Image *im = image;
   if (tofree) *tofree = EINA_FALSE;

   if (!slice || !im)
     return EINA_FALSE;

   if (cspace) *cspace = im->cache_entry.space;
   if (load)
     {
        if (evas_cache_image_load_data(&im->cache_entry) != 0)
          return EINA_FALSE;
     }
   return _evas_common_rgba_image_plane_get(im, plane, slice);
}

/**
 * @brief Sets the colorspace for an image.
 * This may trigger a colorspace conversion when the image data is accessed or drawn.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to modify.
 * @param cspace The target Evas_Colorspace.
 */
static void
eng_image_colorspace_set(void *data EINA_UNUSED, void *image, Evas_Colorspace cspace)
{
   Image_Entry *im;

   if (!image) return;
   im = image;
   evas_cache_image_colorspace(im, cspace);
}

/**
 * @brief Initializes native surface support for a specific type.
 * Currently only supports TBM surfaces.
 * @param data Engine-specific data (unused).
 * @param type The Evas_Native_Surface_Type to initialize.
 * @return 1 on success, 0 on failure or if the type is unsupported.
 */
static int
eng_image_native_init(void *data EINA_UNUSED, Evas_Native_Surface_Type type)
{
   if (type == EVAS_NATIVE_SURFACE_TBM)
     return _evas_native_tbm_init();

   ERR("Native surface type %d not supported!", type);
   return 0;
}

/**
 * @brief Shuts down native surface support for a specific type.
 * Currently only supports TBM surfaces.
 * @param data Engine-specific data (unused).
 * @param type The Evas_Native_Surface_Type to shut down.
 */
static void
eng_image_native_shutdown(void *data EINA_UNUSED, Evas_Native_Surface_Type type)
{
   if (type == EVAS_NATIVE_SURFACE_TBM)
     _evas_native_tbm_shutdown();
   else
     ERR("Native surface type %d not supported!", type);
}

/**
 * @brief Associates a native surface with an Evas image.
 * This allows Evas to potentially use the native surface directly (e.g., for zero-copy).
 * The implementation details depend on the native surface type (TBM, WL_DMABUF, EvasGL).
 * May replace the underlying image data structure.
 * @param data Engine-specific data (unused).
 * @param image The Evas image entry (Image_Entry) to associate with the native surface. Can be NULL for probing.
 * @param native Pointer to the Evas_Native_Surface structure describing the native surface.
 *               If NULL, disassociates any existing native surface.
 * @return The potentially new Evas image entry (Image_Entry) representing the native surface,
 *         or NULL on failure or if disassociating.
 */
static void *
eng_image_native_set(void *data EINA_UNUSED, void *image, void *native)
{
   Evas_Native_Surface *ns = native;
   RGBA_Image *im = image;
   Image_Entry *ie = image, *ie2;

   if (!im)
     {
        /* This is a probe for wl_dmabuf viability */
        if (ns && ns->type == EVAS_NATIVE_SURFACE_WL_DMABUF &&
            !ns->data.wl_dmabuf.resource)
          return _evas_native_dmabuf_surface_image_set(image, native);

        return NULL;
     }
   if (!ns)
     {
        if (im->native.data && im->native.func.free)
          im->native.func.free(im);
        return NULL;
     }

   if ((ns->type == EVAS_NATIVE_SURFACE_EVASGL) &&
       (ns->version == EVAS_NATIVE_SURFACE_VERSION))
     {

        ie2 = evas_cache_image_data(evas_common_image_cache_get(),
                                    ie->w, ie->h,
                                    ns->data.evasgl.surface, 1,
                                    EVAS_COLORSPACE_ARGB8888);
     }
   else
     ie2 = evas_cache_image_data(evas_common_image_cache_get(),
                                 ie->w, ie->h,
                                 NULL, 1,
                                 EVAS_COLORSPACE_ARGB8888);
   if (ie->references > 1)
     ERR("Setting native with more than one references for im=%p", im);

   if (im->native.data)
     {
        if (im->native.func.free)
          im->native.func.free(im);
     }

   evas_cache_image_drop(ie);

   if (ns->type == EVAS_NATIVE_SURFACE_WL_DMABUF)
      return _evas_native_dmabuf_surface_image_set(ie2, ns);

   return ie2;
}

/**
 * @brief Retrieves the native surface associated with an Evas image.
 * @param data Engine-specific data (unused).
 * @param image The Evas image entry (Image_Entry) to query.
 * @return Pointer to the associated Evas_Native_Surface structure, or NULL if none.
 */
static void *
eng_image_native_get(void *data EINA_UNUSED, void *image)
{
   RGBA_Image *im = image;
   Evas_Native_Surface *n;

   if (!im) return NULL;
   n = im->native.data;
   return n;
}

/**
 * @brief Loads an image from a file.
 * Uses the common image loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param file The path to the image file.
 * @param key Optional key within the file (e.g., for Eet files).
 * @param error Pointer to store the Evas_Load_Error code on failure.
 * @param lo Pointer to image loading options.
 * @return A new image entry (Image_Entry) on success, NULL on failure.
 */
static void *
eng_image_load(void *data EINA_UNUSED, const char *file, const char *key, int *error, Evas_Image_Load_Opts *lo)
{
   *error = EVAS_LOAD_ERROR_NONE;

   return evas_common_load_image_from_file(file, key, lo, error);
}

/**
 * @brief Loads an image from a memory-mapped file.
 * Uses the common image loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param f The Eina_File handle representing the memory-mapped file.
 * @param key Optional key within the file.
 * @param error Pointer to store the Evas_Load_Error code on failure.
 * @param lo Pointer to image loading options.
 * @return A new image entry (Image_Entry) on success, NULL on failure.
 */
static void *
eng_image_mmap(void *data EINA_UNUSED, Eina_File *f, const char *key, int *error, Evas_Image_Load_Opts *lo)
{
   *error = EVAS_LOAD_ERROR_NONE;

   return evas_common_load_image_from_mmap(f, key, lo, error);
}

/**
 * @brief Creates a new image entry wrapping existing pixel data (zero-copy).
 * The engine does not own the pixel data and will not free it.
 * @param data Engine-specific data (unused).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param image_data Pointer to the pixel data.
 * @param alpha 1 if the data has alpha, 0 otherwise.
 * @param cspace The Evas_Colorspace of the pixel data.
 * @return A new image entry (Image_Entry) wrapping the data, or NULL on failure.
 */
static void *
eng_image_new_from_data(void *data EINA_UNUSED, int w, int h, DATA32 *image_data, int alpha, Evas_Colorspace cspace)
{
   return evas_cache_image_data(evas_common_image_cache_get(), w, h, image_data, alpha, cspace);
}

/**
 * @brief Creates a new image entry by copying pixel data.
 * The engine allocates its own buffer and copies the provided data into it.
 * @param data Engine-specific data (unused).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param image_data Pointer to the pixel data to copy. If NULL, allocates an uninitialized buffer.
 * @param alpha 1 if the data has alpha, 0 otherwise.
 * @param cspace The Evas_Colorspace of the pixel data.
 * @return A new image entry (Image_Entry) with copied data, or NULL on failure.
 */
static void *
eng_image_new_from_copied_data(void *data EINA_UNUSED, int w, int h, DATA32 *image_data, int alpha, Evas_Colorspace cspace)
{
   return evas_cache_image_copied_data(evas_common_image_cache_get(), w, h, image_data, alpha, cspace);
}

/**
 * @brief Frees an image entry (decrements reference count).
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to free/unref.
 */
static void
eng_image_free(void *data EINA_UNUSED, void *image)
{
   evas_cache_image_drop(image);
}

/**
 * @brief Increments the reference count of an image entry.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to reference.
 * @return The same image entry pointer.
 */
static void *
eng_image_ref(void *data EINA_UNUSED, void *image)
{
   if (!image) return NULL;
   evas_cache_image_ref(image);
   return image;
}

/**
 * @brief Gets the dimensions (width and height) of an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @param w Pointer to store the width.
 * @param h Pointer to store the height.
 */
static void
eng_image_size_get(void *data EINA_UNUSED, void *image, int *w, int *h)
{
   Image_Entry *im;

   im = image;
   if (w) *w = im->w;
   if (h) *h = im->h;
}

/**
 * @brief Sets the dimensions of an image.
 * This might involve reallocating the image buffer or creating a new image entry.
 * Handles potential native surface resource freeing if the image entry changes.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to resize.
 * @param w The new width.
 * @param h The new height.
 * @return The potentially new image entry if reallocation occurred, otherwise the original image.
 */
static void *
eng_image_size_set(void *data EINA_UNUSED, void *image, int w, int h)
{
   RGBA_Image *rm;
   Image_Entry *im = image, *im2;
   if (!im) return NULL;

   /* sw engine im could be changed and removed in evas_cache_image_size_set.
      in this case, there is no chance to free its resource.  */
   evas_cache_image_ref(im);
   im2 = evas_cache_image_size_set(im, w, h);
   if (im != im2 && im->references == 1)
     {
        rm = (RGBA_Image *)im;
        if (rm->native.data)
          {
             if (rm->native.func.free)
               rm->native.func.free(im);
          }
     }
   evas_cache_image_drop(im);
   return im2;
}

/**
 * @brief Marks a region of an image as dirty.
 * This informs the cache that the specified area of the image data has been modified externally.
 * Ensures the image is not shared (copies if necessary) before marking dirty.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to mark dirty.
 * @param x The x-coordinate of the dirty region.
 * @param y The y-coordinate of the dirty region.
 * @param w The width of the dirty region.
 * @param h The height of the dirty region.
 * @return The potentially new image entry if a copy was made, otherwise the original image.
 */
static void *
eng_image_dirty_region(void *data EINA_UNUSED, void *image, int x, int y, int w, int h)
{
   Image_Entry *im = image;
   if (!im) return NULL;
   return evas_cache_image_dirty(im, x, y, w, h);
}

/**
 * @brief Gets a pointer to the raw pixel data of an image.
 * Loads the image data if necessary. If `to_write` is set, ensures the image is
 * not shared (copies if necessary).
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @param to_write 1 if the data will be modified, 0 for read-only access.
 * @param image_data Pointer to store the address of the pixel data (DATA32* for ARGB/GRY, void* for YUV).
 * @param err Pointer to store an Evas_Load_Error code if loading fails.
 * @param tofree Pointer to a boolean that will be set to EINA_TRUE if the caller
 *               needs to free the returned data (currently always EINA_FALSE).
 * @return The potentially new image entry if a copy was made for writing, otherwise the original image.
 *         Returns NULL on failure (e.g., load error, unsupported format for writing).
 */
static void *
eng_image_data_get(void *data EINA_UNUSED, void *image, int to_write, DATA32 **image_data, int *err, Eina_Bool *tofree)
{
   RGBA_Image *im = image;
   int error = EVAS_LOAD_ERROR_NONE;

   *image_data = NULL;
   if (err) *err = EVAS_LOAD_ERROR_NONE;
   if (tofree) *tofree = EINA_FALSE;

   if (!im)
     {
        if (err) *err = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
        return NULL;
     }

   error = evas_cache_image_load_data(&im->cache_entry);

   switch (im->cache_entry.space)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_AGRY88:
      case EVAS_COLORSPACE_GRY8:
	if (to_write)
          im = (RGBA_Image *)evas_cache_image_alone(&im->cache_entry);
	*image_data = im->image.data;
	break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_YCBCR422601_PL:
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        *image_data = im->cs.data;
        break;
      // unlikely formats, not supported for render by the sw engine
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_ETC1_ALPHA:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
        if (to_write)
          {
             ERR("can not get ETC or S3TC data to write");
             *image_data = NULL;
             return NULL;
          }
        *image_data = im->image.data;
        break;
      default:
        CRI("unsupported format %d", im->cache_entry.space);
        if (err) *err = EVAS_LOAD_ERROR_UNKNOWN_FORMAT;
        *image_data = NULL;
        return NULL;
     }
   if (err) *err = error;
   return im;
}

/**
 * @brief Updates the pixel data pointer for an image.
 * If the provided `image_data` pointer is different from the image's current
 * data pointer, it might replace the internal buffer (potentially creating a
 * new image entry for ARGB/GRY formats if the original was zero-copy) or update
 * the YUV data pointer.
 * @param data Engine-specific data (used by eng_image_new_from_data).
 * @param image The image entry (Image_Entry) to update.
 * @param image_data The new pointer to the pixel data.
 * @return The potentially new image entry if a replacement occurred, otherwise the original image.
 *         Returns NULL for unsupported colorspaces.
 */
static void *
eng_image_data_put(void *data, void *image, DATA32 *image_data)
{
   RGBA_Image *im, *im2;

   if (!image) return NULL;
   im = image;
   switch (im->cache_entry.space)
     {
      case EVAS_COLORSPACE_ARGB8888:
      case EVAS_COLORSPACE_AGRY88:
      case EVAS_COLORSPACE_GRY8:
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
      case EVAS_COLORSPACE_ETC1_ALPHA:
      case EVAS_COLORSPACE_RGB_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT1:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT2:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT3:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT4:
      case EVAS_COLORSPACE_RGBA_S3TC_DXT5:
	if (image_data != im->image.data)
	  {
	     int w, h;

	     w = im->cache_entry.w;
	     h = im->cache_entry.h;
	     im2 = eng_image_new_from_data(data, w, h, image_data,
					   eng_image_alpha_get(data, image),
					   eng_image_colorspace_get(data, image));
             evas_cache_image_drop(&im->cache_entry);
	     im = im2;
	  }
	break;
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_YCBCR422601_PL:
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
	if (image_data != im->cs.data)
          {
	     if (im->cs.data)
	       {
		  if (!im->cs.no_free) free(im->cs.data);
	       }
	     im->cs.data = image_data;
	  }
        evas_common_image_colorspace_dirty(im);
        break;
      default:
        CRI("unsupported format %d", im->cache_entry.space);
        return NULL;
     }
   return im;
}

/**
 * @brief Maps a region of an image's pixel data for direct access.
 *
 * Provides access to a portion (or all) of the image data, potentially
 * performing colorspace conversion or copy-on-write as requested.
 *
 * @param engdata Engine-specific data (unused).
 * @param image Pointer to the image entry (Image_Entry*). This might be updated
 *              if copy-on-write occurs.
 * @param slice Pointer to an Eina_Rw_Slice to store the mapped memory region
 *              (pointer and length).
 * @param stride Pointer to store the stride (bytes per row) of the mapped data.
 * @param x The starting x-coordinate of the region to map.
 * @param y The starting y-coordinate of the region to map.
 * @param w The width of the region to map.
 * @param h The height of the region to map.
 * @param cspace The desired Evas_Colorspace for the mapped data. If different
 *               from the image's internal colorspace, conversion will be attempted.
 * @param mode Access mode flags (Efl_Gfx_Buffer_Access_Mode), indicating read/write
 *             and copy-on-write behavior.
 * @param plane The color plane to map (0 for interleaved, 0=Y, 1=U/CbCr, 2=V for planar).
 *              Currently, only plane 0 is fully supported.
 * @return EINA_TRUE on successful mapping, EINA_FALSE on failure (e.g., invalid
 *         parameters, unsupported conversion, allocation error).
 */
static Eina_Bool
eng_image_data_map(void *engdata EINA_UNUSED, void **image, Eina_Rw_Slice *slice,
                   int *stride, int x, int y, int w, int h,
                   Evas_Colorspace cspace, Efl_Gfx_Buffer_Access_Mode mode,
                   int plane)
{
   Eina_Bool cow = EINA_FALSE, to_write = EINA_FALSE;
   RGBA_Image_Data_Map *map;
   RGBA_Image *im;
   Image_Entry *ie;
   int src_stride, src_offset;
   void *data;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(image && *image && slice, EINVAL);
   im = *image;
   ie = &im->cache_entry;

   slice->len = 0;
   slice->mem = NULL;

   // FIXME: implement planes support (YUV, RGB565, ETC1+Alpha)
   // FIXME: implement YUV support (im->cs.data)
   if (plane)
     {
        ERR("planar formats support not implemented yet!");
        return EINA_FALSE;
     }

   if (!im->image.data)
     {
        int error = evas_cache_image_load_data(ie);
        if (error != EVAS_LOAD_ERROR_NONE)
          return EINA_FALSE;
     }

   if (mode & EFL_GFX_BUFFER_ACCESS_MODE_COW)
     cow = EINA_TRUE;

   if (mode & EFL_GFX_BUFFER_ACCESS_MODE_WRITE)
     to_write = EINA_TRUE;

   // verify region is valid regarding special colorspaces (ETC, S3TC, YUV)
   src_offset = _evas_common_rgba_image_data_offset(x, y, w, h, 0, im);
   if ((src_offset < 0) || !w || !h)
     {
        ERR("invalid region for colorspace %d: %dx%d + %d,%d, image: %dx%d",
            cspace, w, h, x, y, ie->w, ie->h);
        return EINA_FALSE;
     }

   src_stride = _evas_common_rgba_image_data_offset(ie->w, 0, 0, 0, 0, im);

   // safety check for COW flag
   EINA_INLIST_FOREACH(im->maps, map)
     {
        if ((!(map->mode & EFL_GFX_BUFFER_ACCESS_MODE_COW)) != (!cow))
          {
             ERR("can't map shared image data multiple times with "
                 "different COW flag");
             return EINA_FALSE;
          }
     }

   // ensure that we are the sole owner of this image entry.
   if (cow)
     {
        ie = evas_cache_image_alone(ie);
        if (!ie) return EINA_FALSE;
        im = (RGBA_Image *) ie;
        *image = im;
     }
   else
     {
        if (to_write && (ie->references > 1))
          {
             ERR("write map requires COW flag for shared images");
             return EINA_FALSE;
          }
     }

   if (cspace != ie->space)
     {
        // using 4x4 "blocks" to support etc1/2, s3tc...
        // note: no actual stride support
        Cspace_Convert_Func cs_func;
        Eina_Bool can_region;
        RGBA_Image fake;
        int rx, ry, rw, rh;
        void *src_data;
        int dst_stride, dst_len, dst_offset = 0;

        cs_func = efl_draw_convert_func_get((Efl_Gfx_Colorspace)ie->space,
                                            (Efl_Gfx_Colorspace)cspace,
                                            &can_region);
        if (!cs_func) return EINA_FALSE;

        // make sure we can convert back, if map for writing
        if (to_write && !efl_draw_convert_func_get((Efl_Gfx_Colorspace)cspace,
                                                   (Efl_Gfx_Colorspace)ie->space,
                                                   NULL))
          return EINA_FALSE;

        if (can_region)
          {
             rx = x;
             ry = y;
             rw = w;
             rh = h;
             src_data = im->image.data8 + src_offset;
          }
        else
          {
             rx = 0;
             ry = 0;
             rw = ie->w;
             rh = ie->h;
             src_data = im->image.data8;
          }

        // a bit hacky, but avoids passing too many parameters to the function
        fake.cache_entry.w = rw;
        fake.cache_entry.h = rh;
        fake.cache_entry.space = cspace;
        dst_stride = _evas_common_rgba_image_data_offset(rw, 0, 0, 0, 0, &fake);
        if (!can_region)
          dst_offset = _evas_common_rgba_image_data_offset(rx, ry, 0, 0, 0, &fake);
        dst_len = rh * dst_stride;

        data = malloc(dst_len);
        if (!data) return EINA_FALSE;

        if (!cs_func(data, src_data, rw, rh, src_stride, dst_stride,
                     ie->flags.alpha,
                     (Efl_Gfx_Colorspace)ie->space,
                     (Efl_Gfx_Colorspace)cspace))
          {
             ERR("color conversion failed");
             free(data);
             return EINA_FALSE;
          }

        map = calloc(1, sizeof(*map));
        if (!map)
          {
             free(data);
             return EINA_FALSE;
          }
        map->allocated = EINA_TRUE;
        map->cspace = cspace;
        map->rx = rx;
        map->ry = ry;
        map->rh = rh;
        map->rw = rw;
        map->mode = mode;
        map->baseptr = data;
        map->slice.mem = map->baseptr + dst_offset;
        map->slice.len = dst_len;
        map->stride = dst_stride;
     }
   else
     {
        // same colorspace
        if (!to_write || !cow)
          {
             // no copy
             int end_offset = _evas_common_rgba_image_data_offset(x + w, y + h, 0, 0, 0, im) - src_stride;
             map = calloc(1, sizeof(*map));
             if (!map) return EINA_FALSE;

             map->baseptr = im->image.data8;
             map->slice.mem = im->image.data8 + src_offset;
             map->slice.len = end_offset - src_offset;
          }
        else
          {
             // copy
             int size = _evas_common_rgba_image_data_offset(w, h, 0, 0, 0, im);
             data = malloc(size);
             if (!data) return EINA_FALSE;
             map = calloc(1, sizeof(*map));
             if (!map)
               {
                  free(data);
                  return EINA_FALSE;
               }
             memcpy(data, im->image.data8 + src_offset, size);

             map->allocated = EINA_TRUE;
             map->baseptr = data;
             map->slice.mem = data;
             map->slice.len = size;
          }
        map->cspace = cspace;
        map->rx = x;
        map->ry = y;
        map->rw = w;
        map->rh = h;
        map->stride = src_stride;
     }

   im->maps = (RGBA_Image_Data_Map *)
     eina_inlist_prepend(EINA_INLIST_GET(im->maps), EINA_INLIST_GET(map));
   if (stride) *stride = map->stride;
   slice->mem = map->slice.mem;
   slice->len = map->slice.len;
   return EINA_TRUE;
}

/**
 * @brief Commits changes made to a mapped region back to the original image buffer.
 * This is called during unmap if the map was allocated (due to COW or colorspace conversion)
 * and opened for writing. It handles potential colorspace conversion back to the
 * image's native format.
 * @param im The target RGBA_Image.
 * @param map The RGBA_Image_Data_Map structure containing the modified data and mapping info.
 */
static void
_image_data_commit(RGBA_Image *im, RGBA_Image_Data_Map *map)
{
   Image_Entry *ie = &im->cache_entry;

   int dst_offset = _evas_common_rgba_image_data_offset(map->rx, map->ry, 0, 0, map->plane, im);
   int dst_stride = _evas_common_rgba_image_data_offset(ie->w, 0, 0, 0, map->plane, im);
   unsigned char *dst = im->image.data8 + dst_offset;

   if (map->cspace == ie->space)
     {
        if (dst_stride == (int) map->stride)
          {
             DBG("unmap commit: single memcpy");
             memcpy(dst, map->slice.bytes, dst_stride * map->rh);
          }
        else
          {
             DBG("unmap commit: multiple memcpy");
             for (int k = 0; k < dst_stride; k++)
               memcpy(dst + k * dst_stride, map->slice.bytes + k * dst_stride, dst_stride);
          }
     }
   else
     {
        Cspace_Convert_Func cs_func;
        Eina_Bool can_region;

        cs_func = efl_draw_convert_func_get((Efl_Gfx_Colorspace)map->cspace,
                                            (Efl_Gfx_Colorspace)ie->space,
                                            &can_region);
        EINA_SAFETY_ON_NULL_RETURN(cs_func);

        DBG("unmap commit: convert func (%p)", cs_func);
        if (can_region)
          {
             cs_func(dst, map->slice.mem, map->rw, map->rh, map->stride, dst_stride,
                     ie->flags.alpha,
                     (Efl_Gfx_Colorspace)map->cspace,
                     (Efl_Gfx_Colorspace)ie->space);
          }
        else
          {
             cs_func(dst, map->baseptr, ie->w, ie->h, map->stride, dst_stride,
                     ie->flags.alpha,
                     (Efl_Gfx_Colorspace)map->cspace,
                     (Efl_Gfx_Colorspace)ie->space);
          }
     }
}

/**
 * @brief Unmaps a previously mapped image data region.
 * Finds the corresponding map entry based on the slice, performs commit if necessary,
 * frees allocated resources (if any), and removes the map entry.
 * @param engdata Engine-specific data (unused).
 * @param image The image entry (Image_Entry) that was mapped.
 * @param slice The Eina_Rw_Slice representing the region to unmap (must match the one returned by map).
 * @return EINA_TRUE on success, EINA_FALSE if the slice doesn't correspond to a known map.
 */
static Eina_Bool
eng_image_data_unmap(void *engdata EINA_UNUSED, void *image, const Eina_Rw_Slice *slice)
{
   RGBA_Image_Data_Map *map;
   RGBA_Image *im = image;

   if (!(image && slice))
     return EINA_FALSE;

   EINA_INLIST_FOREACH(EINA_INLIST_GET(im->maps), map)
     {
        if ((map->slice.len == slice->len) && (map->slice.mem == slice->mem))
          {
             if (map->allocated)
               {
                  if (map->mode & EFL_GFX_BUFFER_ACCESS_MODE_WRITE)
                    _image_data_commit(im, map);
                  free(map->baseptr);
               }
             im->maps = (RGBA_Image_Data_Map *)
                   eina_inlist_remove(EINA_INLIST_GET(im->maps), EINA_INLIST_GET(map));
             free(map);
             return EINA_TRUE;
          }
     }

   ERR("failed to unmap region %p (%zu bytes)", slice->mem, slice->len);
   return EINA_FALSE;
}

/**
 * @brief Retrieves all currently active data maps for an image.
 * @param engdata Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to query.
 * @param slices If not NULL, an array to be filled with pointers to the Eina_Rw_Slice
 *               structures representing the active maps. The array must be large enough.
 *               If NULL, the function only returns the count.
 * @return The number of active maps, or -1 if image is NULL.
 */
static int
eng_image_data_maps_get(void *engdata EINA_UNUSED, const void *image, const Eina_Rw_Slice **slices)
{
   RGBA_Image_Data_Map *map;
   const RGBA_Image *im = image;
   int k = 0;

   if (!im) return -1;

   if (!slices)
     return eina_inlist_count(EINA_INLIST_GET(im->maps));

   EINA_INLIST_FOREACH(EINA_INLIST_GET(im->maps), map)
     slices[k++] = &map->slice;

   return k;
}

/**
 * @brief Helper function to check if a colorspace is a YUV format handled by this engine.
 * @param cspace The Evas_Colorspace to check.
 * @return EINA_TRUE if it's a supported YUV format, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_is_yuv(Evas_Colorspace cspace)
{
   switch (cspace)
     {
      case EVAS_COLORSPACE_YCBCR422P601_PL:
      case EVAS_COLORSPACE_YCBCR422P709_PL:
      case EVAS_COLORSPACE_YCBCR422601_PL:
      case EVAS_COLORSPACE_YCBCR420NV12601_PL:
      case EVAS_COLORSPACE_YCBCR420TM12601_PL:
        return EINA_TRUE;

      default:
        return EINA_FALSE;
     }
}

/**
 * @brief Adds or updates image data using an Eina_Slice, typically for a specific plane.
 *
 * This function is intended for setting image data plane by plane, especially
 * for YUV formats or potentially other planar/complex formats in the future.
 * It can create a new image or update an existing one.
 *
 * @note This function is not robust and should NOT be mixed with eng_image_data_get/put.
 *       It assumes parameters like w, h, cspace, alpha are correct.
 *       Zero-copy (`copy = EINA_FALSE`) is only supported for specific formats and
 *       requires the stride to match the expected packed stride. Copying YUV data
 *       is currently not implemented.
 *
 * @param engdata Engine-specific data (used for image creation).
 * @param image The existing image entry (Image_Entry) to update, or NULL to create a new one.
 *              If updating, the image might be modified (e.g., made non-shared).
 * @param slice The Eina_Slice containing the pixel data for the plane.
 * @param copy If EINA_TRUE, the data from the slice is copied into the image's
 *             internal buffer. If EINA_FALSE, attempts zero-copy (buffer pointer assignment).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param stride Stride (bytes per row) of the data in the slice. If 0, calculated based on w and cspace/bpp.
 * @param cspace The Evas_Colorspace of the data in the slice.
 * @param plane The plane index this slice represents (0 for interleaved, 0=Y, 1=Cb, 2=Cr, etc.).
 * @param alpha Boolean indicating if the image should have an alpha channel.
 * @return The updated or newly created image entry (Image_Entry), or NULL on failure
 *         (e.g., invalid parameters, unsupported format/operation, allocation error).
 */
static void *
eng_image_data_slice_add(void *engdata, void *image,
                         const Eina_Slice *slice, Eina_Bool copy,
                         int w, int h, int stride, Evas_Colorspace cspace,
                         int plane, Eina_Bool alpha)
{
   const Eina_Bool use_cs = _is_yuv(cspace);
   const unsigned char **cs_data;
   RGBA_Image *im = image;
   int bpp = 0;

   // Note: This code is not very robust by choice. It should NOT be used
   // in conjunction with data_put/data_get. Ever.
   // Assume w,h,cspace,alpha to be correct.
   // We still use cs.data for YUV.
   // 'image' may be NULL, in that case create a new one. Otherwise, it must
   // have been created by a previous call to this function.

   if ((plane < 0) || (plane >= RGBA_PLANE_MAX)) goto fail;
   if (!slice || !slice->mem) goto fail;
   copy = !!copy;

   // not implemented
   if (use_cs && copy)
     {
        // To implement this, we should switch the internals to slices first,
        // as this would give 3 planes rather than N rows of datas
        ERR("Evas can not copy YUV data (not implemented yet).");
        goto fail;
     }

   // alloc
   if (!im)
     {
        switch (cspace)
          {
           case EFL_GFX_COLORSPACE_ARGB8888:
           case EFL_GFX_COLORSPACE_AGRY88:
           case EFL_GFX_COLORSPACE_GRY8:
             if (plane != 0) goto fail;
             if (copy)
               im = eng_image_new_from_copied_data(engdata, w, h, NULL, alpha, cspace);
             else
               im = eng_image_new_from_data(engdata, w, h, NULL, alpha, cspace);
             break;

           case EFL_GFX_COLORSPACE_YCBCR422P601_PL:
           case EFL_GFX_COLORSPACE_YCBCR422P709_PL:
           case EFL_GFX_COLORSPACE_YCBCR422601_PL:
           case EFL_GFX_COLORSPACE_YCBCR420NV12601_PL:
             // Use 'copied' to allocate the RGBA buffer
             im = eng_image_new_from_copied_data(engdata, w, h, NULL, alpha, cspace);
             break;

           default:
             // TODO: ETC, S3TC, YCBCR420TM12 (aka ST12 or tiled NV12)
             goto fail;
          }
        if (!im) goto fail;
     }
   else
     {
        im = (RGBA_Image *) evas_cache_image_alone(&im->cache_entry);
        if (!im) goto fail;
     }

   if (use_cs && (!im->cs.data || im->cs.no_free))
     {
        im->cs.data = calloc(1, h * sizeof(void *) * 2);
        if (!im->cs.data) goto fail;
        im->cs.no_free = EINA_FALSE;
     }

   // assign
   switch (cspace)
     {
      case EFL_GFX_COLORSPACE_ARGB8888:
        bpp = 4;
        EINA_FALLTHROUGH;
      case EFL_GFX_COLORSPACE_AGRY88:
        if (!bpp) bpp = 2;
        EINA_FALLTHROUGH;
      case EFL_GFX_COLORSPACE_GRY8:
        if (!bpp) bpp = 1;
        if (plane != 0) goto fail;
        if (!stride) stride = w * bpp;
        if (copy)
          {
             for (int y = 0; y < h; y++)
               {
                  const unsigned char *src = slice->bytes + h * stride;
                  unsigned char *dst = im->image.data8 + bpp * w;
                  memcpy(dst, src, w * bpp);
               }
          }
        else
          {
             if (stride != (bpp * w))
               {
                  ERR("invalid stride for zero-copy data set");
                  goto fail;
               }
             im->image.data = (DATA32 *) slice->mem;
             im->image.no_free = EINA_TRUE;
          }
        break;

      case EFL_GFX_COLORSPACE_YCBCR422P601_PL:
      case EFL_GFX_COLORSPACE_YCBCR422P709_PL:
        /* YCbCr 4:2:2 Planar: Y rows, then the Cb, then Cr rows. */
        cs_data = im->cs.data;
        if (plane == 0)
          {
             if (!stride) stride = w;
             for (int y = 0; y < h; y++)
               cs_data[y] = slice->bytes + (y * stride);
          }
        else if (plane == 1)
          {
             if (!stride) stride = w / 2;
             for (int y = 0; y < (h / 2); y++)
               cs_data[h + y] = slice->bytes + (y * stride);
          }
        else if (plane == 2)
          {
             if (!stride) stride = w / 2;
             for (int y = 0; y < (h / 2); y++)
               cs_data[h + (h / 2) + y] = slice->bytes + (y * stride);
          }
        evas_common_image_colorspace_dirty(im);
        break;

      case EFL_GFX_COLORSPACE_YCBCR422601_PL:
        /* YCbCr 4:2:2: lines of Y,Cb,Y,Cr bytes. */
        if (plane != 0) goto fail;
        if (!stride) stride = w * 2;
        cs_data = im->cs.data;
        for (int y = 0; y < h; y++)
          cs_data[y] = slice->bytes + (y * stride);
        evas_common_image_colorspace_dirty(im);
        break;

      case EFL_GFX_COLORSPACE_YCBCR420NV12601_PL:
        /* YCbCr 4:2:0: Y rows, then the Cb,Cr rows. */
        if (!stride) stride = w;
        cs_data = im->cs.data;
        if (plane == 0)
          {
             for (int y = 0; y < h; y++)
               cs_data[y] = slice->bytes + (y * stride);
          }
        else if (plane == 1)
          {
             for (int y = 0; y < (h / 2); y++)
               cs_data[h + y] = slice->bytes + (y * stride);
          }
        evas_common_image_colorspace_dirty(im);
        break;

        // ETC, S3TC, YCBCR420TM12 (aka ST12 or tiled NV12)
      default:
        ERR("unsupported color space %d", cspace);
        goto fail;
     }

   return im;

fail:
   if (im) eng_image_free(engdata, im);
   return NULL;
}

/**
 * @brief Hint to the engine to prepare an image for rendering.
 * For the software engine, this is currently a no-op, but could potentially
 * trigger background loading threads.
 * @param engdata Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to prepare (unused).
 */
static void
eng_image_prepare(void *engdata EINA_UNUSED, void *image EINA_UNUSED)
{
   // software rendering doesnt want/need to prepare at this point
   // XXX: though this could push along any loading threads or start
   // some thread jobs for loading in the bg.
}

/**
 * @brief Creates a new image surface intended for direct rendering without scaling.
 * In the software engine, this is equivalent to creating a standard buffer for map surfaces.
 * @param engdata Engine-specific data.
 * @param w Width of the surface.
 * @param h Height of the surface.
 * @param alpha Boolean indicating if the surface should support alpha.
 * @return A pointer to the newly created RGBA_Image surface, or NULL on failure.
 */
static void *
eng_image_surface_noscale_new(void *engdata, int w, int h, int alpha)
{
   // simply call the map surface new as all we need is a basic buffer
   return eng_image_map_surface_new(engdata, w, h, alpha);
}

/**
 * @brief Flips an ARGB image horizontally. Operates in-place if pixels_out == pixels_in.
 * @param pixels_out Destination pixel buffer.
 * @param pixels_in Source pixel buffer.
 * @param iw Image width.
 * @param ih Image height.
 */
static void
_image_flip_horizontal(DATA32 *pixels_out, const DATA32 *pixels_in,
                       int iw, int ih)
{
   const unsigned int *pi1, *pi2;
   unsigned int *po1, *po2;
   int x, y;

   for (y = 0; y < ih; y++)
     {
        pi1 = pixels_in + (y * iw);
        pi2 = pixels_in + ((y + 1) * iw) - 1;
        po1 = pixels_out + (y * iw);
        po2 = pixels_out + ((y + 1) * iw) - 1;
        for (x = 0; x < (iw >> 1); x++)
          {
             *po2 = *pi1;
             *po1 = *pi2;
             pi1++; po1++;
             pi2--; po2--;
          }
     }
}

/**
 * @brief Flips an ARGB image vertically. Operates in-place if pixels_out == pixels_in.
 * @param pixels_out Destination pixel buffer.
 * @param pixels_in Source pixel buffer.
 * @param iw Image width.
 * @param ih Image height.
 */
static void
_image_flip_vertical(DATA32 *pixels_out, const DATA32 *pixels_in,
                     int iw, int ih)
{
   const unsigned int *pi1, *pi2;
   unsigned int *po1, *po2;
   int x, y;

   for (y = 0; y < (ih >> 1); y++)
     {
        pi1 = pixels_in + (y * iw);
        pi2 = pixels_in + ((ih - 1 - y) * iw);
        po1 = pixels_out + (y * iw);
        po2 = pixels_out + ((ih - 1 - y) * iw);
        for (x = 0; x < iw; x++)
          {
             *po2 = *pi1;
             *po1 = *pi2;
             pi1++; po1++;
             pi2++; po2++;
          }
     }
}

/**
 * @brief Rotates an ARGB image by 180 degrees. Operates in-place if pixels_out == pixels_in.
 * @param pixels_out Destination pixel buffer.
 * @param pixels_in Source pixel buffer.
 * @param iw Image width.
 * @param ih Image height.
 */
static void
_image_rotate_180(DATA32 *pixels_out, const DATA32 *pixels_in,
                  int iw, int ih)
{
   const unsigned int *pi1, *pi2;
   unsigned int *po1, *po2;
   int hw;

   hw = iw * ih;
   pi1 = pixels_in;
   pi2 = pixels_in + hw - 1;
   po1 = pixels_out;
   po2 = pixels_out + hw - 1;
   for (; pi1 < pi2; )
     {
        *po2 = *pi1;
        *po1 = *pi2;
        pi1++; po1++;
        pi2--; po2--;
     }
}

/**
 * @brief Rotates an ARGB image by 90 degrees clockwise. Requires separate buffers.
 * Uses tiling for potentially better cache performance.
 * @param pixels_out Destination pixel buffer (must be ih x iw).
 * @param pixels_in Source pixel buffer (iw x ih).
 * @param iw Source image width.
 * @param ih Source image height.
 */
static void
_image_rotate_90(DATA32 *pixels_out, const DATA32 *pixels_in, int iw, int ih)
{
   int x, y, xx, yy, xx2, yy2;

   for (y = 0; y < ih; y += TILE)
     {
        yy2 = y + TILE;
        if (yy2 > ih) yy2 = ih;
        for (x = 0; x < iw; x += TILE)
          {
             xx2 = x + TILE;
             if (xx2 > iw) xx2 = iw;
             for (yy = y; yy < yy2; yy++)
               {
                  const unsigned int *src;
                  unsigned int *dst;

                  src = pixels_in + (yy * iw) + x;
                  dst = pixels_out + (x * ih) + (ih - yy - 1);
                  for (xx = x; xx < xx2; xx++)
                    {
                       *dst = *src;
                       src++;
                       dst += ih;
                    }
               }
          }
     }
}

/**
 * @brief Rotates an ARGB image by 270 degrees clockwise (90 counter-clockwise). Requires separate buffers.
 * Uses tiling for potentially better cache performance.
 * @param pixels_out Destination pixel buffer (must be ih x iw).
 * @param pixels_in Source pixel buffer (iw x ih).
 * @param iw Source image width.
 * @param ih Source image height.
 */
static void
_image_rotate_270(DATA32 *pixels_out, const DATA32 *pixels_in, int iw, int ih)
{
   int x, y, xx, yy, xx2, yy2;

   for (y = 0; y < ih; y += TILE)
     {
        yy2 = y + TILE;
        if (yy2 > ih) yy2 = ih;
        for (x = 0; x < iw; x += TILE)
          {
             xx2 = x + TILE;
             if (xx2 > iw) xx2 = iw;
             for (yy = y; yy < yy2; yy++)
               {
                  const unsigned int *src;
                  unsigned int *dst;

                  src = pixels_in + (yy * iw) + x;
                  dst = pixels_out + ((iw - x - 1) * ih) + yy;
                  for (xx = x; xx < xx2; xx++)
                    {
                       *dst = *src;
                       src++;
                       dst -= ih;
                    }
               }
          }
     }
}

/**
 * @brief Transposes an ARGB image (flips along the top-left to bottom-right diagonal). Requires separate buffers.
 * @param pixels_out Destination pixel buffer (must be ih x iw).
 * @param pixels_in Source pixel buffer (iw x ih).
 * @param iw Source image width.
 * @param ih Source image height.
 */
static void
_image_flip_transpose(DATA32 *pixels_out, const DATA32 *pixels_in,
                      int iw, int ih)
{
   int x, y;
   const unsigned int *src;

   src = pixels_in;
   for (y = 0; y < ih; y++)
     {
        unsigned int *dst;

        dst = pixels_out + y;
        for (x = 0; x < iw; x++)
          {
             unsigned int tmp = *src;
             *dst = tmp;
             src++;
             dst += ih;
          }
     }
}

/**
 * @brief Transverses an ARGB image (flips along the top-right to bottom-left diagonal). Requires separate buffers.
 * @param pixels_out Destination pixel buffer (must be ih x iw).
 * @param pixels_in Source pixel buffer (iw x ih).
 * @param iw Source image width.
 * @param ih Source image height.
 */
static void
_image_flip_transverse(DATA32 *pixels_out, const DATA32 *pixels_in,
                       int iw, int ih)
{
   int x, y;
   const unsigned int *src;

   src = pixels_in + (iw * ih) - 1;
   for (y = 0; y < ih; y++)
     {
        unsigned int *dst;

        dst = pixels_out + y;
        for (x = 0; x < iw; x++)
          {
             *dst = *src;
             src--;
             dst += ih;
          }
     }
}

/**
 * @brief Sets the orientation of an image.
 * This function physically transforms the pixel data according to the new orientation.
 * It creates a new image entry with the transformed data and drops the old one.
 * Handles transitions between different orientations efficiently where possible.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to reorient.
 * @param orient The target Evas_Image_Orient value.
 * @return The new image entry with the specified orientation, or the original image if
 *         the orientation is unchanged or an error occurred.
 */
static void *
eng_image_orient_set(void *data EINA_UNUSED, void *image, Evas_Image_Orient orient)
{
   Image_Entry *im;
   Image_Entry *im_new;
   void *pixels_in;
   void *pixels_out;
   int tw, th;
   int w, h;

   if (!image) return NULL;
   im = image;
   if (im->orient == orient) return im;

   if (im->orient == EVAS_IMAGE_ORIENT_90 ||
       im->orient == EVAS_IMAGE_ORIENT_270 ||
       im->orient == EVAS_IMAGE_FLIP_TRANSPOSE ||
       im->orient == EVAS_IMAGE_FLIP_TRANSVERSE)
     {
        tw = im->h;
        th = im->w;
     }
   else
     {
        th = im->h;
        tw = im->w;
     }

   if (orient == EVAS_IMAGE_ORIENT_90 ||
       orient == EVAS_IMAGE_ORIENT_270 ||
       orient == EVAS_IMAGE_FLIP_TRANSPOSE ||
       orient == EVAS_IMAGE_FLIP_TRANSVERSE)
     {
        w = th;
        h = tw;
     }
   else
     {
        h = th;
        w = tw;
     }

   im_new = evas_cache_image_copied_data(evas_common_image_cache_get(),
                                         w, h, NULL, im->flags.alpha,
                                         EVAS_COLORSPACE_ARGB8888);
   if (!im_new) return im;

   evas_cache_image_load_data(im);

   pixels_in = evas_cache_image_pixels(im);
   pixels_out = evas_cache_image_pixels(im_new);

   if (!pixels_out || !pixels_in) goto on_error;

   if ((im->orient <= EVAS_IMAGE_ORIENT_270) &&
       (orient <= EVAS_IMAGE_ORIENT_270))
     {
        // we are rotating from one anglee to another, so figure out delta
        // and apply that delta
        Evas_Image_Orient rot_delta = (4 + orient - im->orient) % 4;
        switch (rot_delta)
          {
           case EVAS_IMAGE_ORIENT_0:
              ERR("You shouldn't get this message, wrong orient value");
              goto on_error;
           case EVAS_IMAGE_ORIENT_90:
              _image_rotate_90(pixels_out, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_ORIENT_180:
              _image_rotate_180(pixels_out, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_ORIENT_270:
              _image_rotate_270(pixels_out, pixels_in, im->w, im->h);
              break;
           default:
              ERR("Wrong orient value");
              goto on_error;
          }
     }
   else if (((im->orient == EVAS_IMAGE_ORIENT_NONE) &&
             (orient == EVAS_IMAGE_FLIP_HORIZONTAL)) ||
            ((im->orient == EVAS_IMAGE_FLIP_HORIZONTAL) &&
             (orient == EVAS_IMAGE_ORIENT_NONE)))
     {
        // flip horizontally to get the new orientation
        _image_flip_horizontal(pixels_out, pixels_in, im->w, im->h);
     }
   else if (((im->orient == EVAS_IMAGE_ORIENT_NONE) &&
             (orient == EVAS_IMAGE_FLIP_VERTICAL)) ||
            ((im->orient == EVAS_IMAGE_FLIP_VERTICAL) &&
             (orient == EVAS_IMAGE_ORIENT_NONE)))
     {
        // flip vertically to get the new orientation
        _image_flip_vertical(pixels_out, pixels_in, im->w, im->h);
     }
   else
     {
        // generic solution - undo the previous orientation and then apply the
        // new one after that
        void *pixels_tmp;

        pixels_tmp = malloc(sizeof (unsigned int) * w * h);
        if (!pixels_tmp) goto on_error;

        // Undoing previous rotation
        switch (im->orient)
          {
           case EVAS_IMAGE_ORIENT_0:
              // FIXME: could be easily optimized away
              memcpy(pixels_tmp, pixels_in, sizeof (unsigned int) * w * h);
              break;
           case EVAS_IMAGE_ORIENT_90:
              _image_rotate_270(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_ORIENT_180:
              _image_rotate_180(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_ORIENT_270:
              _image_rotate_90(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_FLIP_HORIZONTAL:
              _image_flip_horizontal(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_FLIP_VERTICAL:
              _image_flip_vertical(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_FLIP_TRANSPOSE:
              _image_flip_transpose(pixels_tmp, pixels_in, im->w, im->h);
              break;
           case EVAS_IMAGE_FLIP_TRANSVERSE:
              _image_flip_transverse(pixels_tmp, pixels_in, im->w, im->h);
              break;
           default:
              ERR("Wrong orient value");
              free(pixels_tmp);
              goto on_error;
          }

        // Doing the new requested one
        switch (orient)
          {
           case EVAS_IMAGE_ORIENT_0:
              // FIXME: could be easily optimized away
              memcpy(pixels_out, pixels_tmp, sizeof (unsigned int) * w * h);
              break;
           case EVAS_IMAGE_ORIENT_90:
              _image_rotate_90(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_ORIENT_180:
              _image_rotate_180(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_ORIENT_270:
              _image_rotate_270(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_FLIP_HORIZONTAL:
              _image_flip_horizontal(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_FLIP_VERTICAL:
              _image_flip_vertical(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_FLIP_TRANSPOSE:
              _image_flip_transpose(pixels_out, pixels_tmp, tw, th);
              break;
           case EVAS_IMAGE_FLIP_TRANSVERSE:
              _image_flip_transverse(pixels_out, pixels_tmp, tw, th);
              break;
          }

        free(pixels_tmp);
     }

   im_new->orient = orient;
   evas_cache_image_drop(im);

   return im_new;

 on_error:
   evas_cache_image_drop(im_new);
   return im;
}

/**
 * @brief Gets the current orientation of an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The current Evas_Image_Orient value.
 */
static Evas_Image_Orient
eng_image_orient_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EVAS_IMAGE_ORIENT_NONE;
   im = image;
   return im->orient;
}

/**
 * @brief Requests asynchronous preloading of image data.
 * The actual loading happens in a separate thread (managed by evas_cache).
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to preload.
 * @param target The target Evas object associated with this preload request (used for cancellation).
 */
static void
eng_image_data_preload_request(void *data EINA_UNUSED, void *image, const Eo *target)
{
   RGBA_Image *im = image;
   if (!im) return;

   evas_cache_image_preload_data(&im->cache_entry, target, NULL, NULL);
}

/**
 * @brief Cancels an image data preload request associated with a target object.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) whose preload should be cancelled.
 * @param target The target Evas object used in the preload request.
 * @param force If EINA_TRUE, cancel immediately even if loading is in progress.
 */
static void
eng_image_data_preload_cancel(void *data EINA_UNUSED, void *image, const Eo *target, Eina_Bool force)
{
   RGBA_Image *im = image;

   if (!im) return;

   evas_cache_image_preload_cancel(&im->cache_entry, target, force);
}

/**
 * @brief Executes an image drawing command in a rendering thread.
 * Handles smooth (bilinear) or sample (nearest neighbor) scaling.
 * This function is called by the thread pool.
 * @param data Pointer to an Evas_Thread_Command_Image structure containing drawing parameters.
 */
static void
_draw_thread_image_draw(void *data)
{
   Evas_Thread_Command_Image *image = data;

   if (image->smooth)
     evas_common_scale_rgba_smooth_draw
       (image->image, image->surface,
        image->clip.x, image->clip.y, image->clip.w, image->clip.h,
        image->mul_col, image->render_op,
        image->src.x, image->src.y, image->src.w, image->src.h,
        image->dst.x, image->dst.y, image->dst.w, image->dst.h,
        image->mask, image->mask_x, image->mask_y);
   else
     evas_common_scale_rgba_sample_draw
       (image->image, image->surface,
        image->clip.x, image->clip.y, image->clip.w, image->clip.h,
        image->mul_col, image->render_op,
        image->src.x, image->src.y, image->src.w, image->src.h,
        image->dst.x, image->dst.y, image->dst.w, image->dst.h,
        image->mask, image->mask_x, image->mask_y);

   eina_mempool_free(_mp_command_image, image);
}

/**
 * @brief Creates and enqueues an image drawing command for threaded execution.
 * Performs clipping based on destination bounds and context clip settings.
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The drawing context.
 * @param src_x Source rectangle x-coordinate.
 * @param src_y Source rectangle y-coordinate.
 * @param src_w Source rectangle width.
 * @param src_h Source rectangle height.
 * @param dst_x Destination rectangle x-coordinate.
 * @param dst_y Destination rectangle y-coordinate.
 * @param dst_w Destination rectangle width.
 * @param dst_h Destination rectangle height.
 * @param smooth 1 for smooth (bilinear) scaling, 0 for sample (nearest).
 * @return EINA_TRUE if the command was successfully enqueued, EINA_FALSE otherwise.
 */
static Eina_Bool
_image_draw_thread_cmd(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h, int smooth)
{
   Evas_Thread_Command_Image *cr;
   int clip_x, clip_y, clip_w, clip_h;

   if ((dst_w <= 0) || (dst_h <= 0)) return EINA_FALSE;
   if (!(RECTS_INTERSECT(dst_x, dst_y, dst_w, dst_h,
                         0, 0, dst->cache_entry.w, dst->cache_entry.h))) return EINA_FALSE;

   cr = eina_mempool_malloc(_mp_command_image, sizeof (Evas_Thread_Command_Image));
   if (!cr) return EINA_FALSE;

   cr->image = src;
   cr->surface = dst;
   EINA_RECTANGLE_SET(&cr->src, src_x, src_y, src_w, src_h);
   EINA_RECTANGLE_SET(&cr->dst, dst_x, dst_y, dst_w, dst_h);

   if (dc->clip.use)
     {
	clip_x = dc->clip.x;
	clip_y = dc->clip.y;
	clip_w = dc->clip.w;
	clip_h = dc->clip.h;
     }
   else
     {
	clip_x = 0;
	clip_y = 0;
	clip_w = dst->cache_entry.w;
	clip_h = dst->cache_entry.h;
     }

   /* Set image mask, if any */
   cr->mask = dc->clip.mask;
   cr->mask_x = dc->clip.mask_x;
   cr->mask_y = dc->clip.mask_y;
   if (cr->mask)
     {
        Image_Entry *im = cr->mask;
        RECTS_CLIP_TO_RECT(clip_x, clip_y, clip_w, clip_h,
                           cr->mask_x, cr->mask_y,
                           im->w, im->h);
     }

   EINA_RECTANGLE_SET(&cr->clip, clip_x, clip_y, clip_w, clip_h);

   cr->mul_col = dc->mul.use ? dc->mul.col : 0xffffffff;
   cr->render_op = dc->render_op;
   cr->smooth = smooth;

   QCMD(_draw_thread_image_draw, cr);

   return EINA_TRUE;
}

/**
 * @brief Helper to enqueue a smooth image drawing command.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_image_draw_thread_cmd_smooth(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   return _image_draw_thread_cmd(src, dst, dc,
                                 src_x, src_y, src_w, src_h,
                                 dst_x, dst_y, dst_w, dst_h,
                                 1);
}

/**
 * @brief Helper to enqueue a sample (non-smooth) image drawing command.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_image_draw_thread_cmd_sample(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   return _image_draw_thread_cmd(src, dst, dc,
                                 src_x, src_y, src_w, src_h,
                                 dst_x, dst_y, dst_w, dst_h,
                                 0);
}

/**
 * @brief Callback function used by the scale cache mechanism for smooth threaded drawing.
 * This function is passed to evas_common_rgba_image_scalecache_do_cbs.
 * It clips the drawing operation and enqueues the actual drawing command.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_image_thr_cb_smooth(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   return evas_common_scale_rgba_in_to_out_clip_cb(src, dst, dc,
                                                   src_x, src_y, src_w, src_h,
                                                   dst_x, dst_y, dst_w, dst_h,
                                                   _image_draw_thread_cmd_smooth);
}

/**
 * @brief Callback function used by the scale cache mechanism for sample (non-smooth) threaded drawing.
 * This function is passed to evas_common_rgba_image_scalecache_do_cbs.
 * It clips the drawing operation and enqueues the actual drawing command.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_image_thr_cb_sample(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   return evas_common_scale_rgba_in_to_out_clip_cb(src, dst, dc,
                                                   src_x, src_y, src_w, src_h,
                                                   dst_x, dst_y, dst_w, dst_h,
                                                   _image_draw_thread_cmd_sample);
}

/**
 * @brief Engine function to draw an image (potentially scaled).
 * Dispatches the drawing to the appropriate implementation (sync, async thread, pipe),
 * potentially utilizing the scale cache. Handles native surface binding/unbinding.
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param image The source image entry (Image_Entry).
 * @param src_x Source rectangle x-coordinate.
 * @param src_y Source rectangle y-coordinate.
 * @param src_w Source rectangle width.
 * @param src_h Source rectangle height.
 * @param dst_x Destination rectangle x-coordinate.
 * @param dst_y Destination rectangle y-coordinate.
 * @param dst_w Destination rectangle width.
 * @param dst_h Destination rectangle height.
 * @param smooth 1 for smooth (bilinear) scaling, 0 for sample (nearest).
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 * @return EINA_TRUE if drawing was handled (typically for async), EINA_FALSE otherwise (sync or error).
 */
static Eina_Bool
eng_image_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, void *image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h, int smooth, Eina_Bool do_async)
{
   RGBA_Image *im;

   if (!image) return EINA_FALSE;
   im = image;
   if (im->native.func.bind)
      im->native.func.bind(image, src_x, src_y, src_w, src_h);

   if (do_async)
     {
        Eina_Bool ret;
        if (!evas_common_rgba_image_scalecache_prepare(image, surface, context, smooth,
                                                       src_x, src_y, src_w, src_h,
                                                       dst_x, dst_y, dst_w, dst_h))
          {
             if (im->cache_entry.space == EVAS_COLORSPACE_ARGB8888)
               {
                  evas_cache_image_load_data(&im->cache_entry);
                  if (!im->cache_entry.flags.loaded)
                    {
                       if (im->native.func.unbind)
                         im->native.func.unbind(image);
                       return EINA_FALSE;
                    }
               }
          }
        ret = evas_common_rgba_image_scalecache_do_cbs(image, surface,
                                                        context, smooth,
                                                        src_x, src_y, src_w, src_h,
                                                        dst_x, dst_y, dst_w, dst_h,
                                                        _image_thr_cb_sample,
                                                        _image_thr_cb_smooth);
        if (im->native.func.unbind)
           im->native.func.unbind(image);
        return ret;
     }
#ifdef BUILD_PIPE_RENDER
   else if ((cpunum > 1))
     {
        evas_common_rgba_image_scalecache_prepare((Image_Entry *)(im),
                                                  surface, context, smooth,
                                                  src_x, src_y, src_w, src_h,
                                                  dst_x, dst_y, dst_w, dst_h);

        evas_common_pipe_image_draw(im, surface, context, smooth,
                                    src_x, src_y, src_w, src_h,
                                    dst_x, dst_y, dst_w, dst_h);
     }
#endif
   else
     {
        evas_common_rgba_image_scalecache_prepare
          (&im->cache_entry, surface, context, smooth,
           src_x, src_y, src_w, src_h,
           dst_x, dst_y, dst_w, dst_h);
        evas_common_rgba_image_scalecache_do
          (&im->cache_entry, surface, context, smooth,
           src_x, src_y, src_w, src_h,
           dst_x, dst_y, dst_w, dst_h);

        evas_common_cpu_end_opt();
     }

   if (im->native.func.unbind)
      im->native.func.unbind(image);
   return EINA_FALSE;
}

/**
 * @brief Internal function to draw a scaled image, used as a callback for map drawing optimization.
 * This is called when a map operation degenerates into a simple scale/blit.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @param smooth Smooth scaling flag.
 */
static void
_map_image_draw(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h, int smooth)
{
   int clip_x, clip_y, clip_w, clip_h;
   DATA32 mul_col;

   if ((dst_w <= 0) || (dst_h <= 0)) return;
   if (!(RECTS_INTERSECT(dst_x, dst_y, dst_w, dst_h,
                         0, 0, dst->cache_entry.w, dst->cache_entry.h))) return;

   if (dc->clip.use)
     {
	clip_x = dc->clip.x;
	clip_y = dc->clip.y;
	clip_w = dc->clip.w;
	clip_h = dc->clip.h;
     }
   else
     {
	clip_x = clip_y = 0;
	clip_w = dst->cache_entry.w;
	clip_h = dst->cache_entry.h;
     }

   mul_col = dc->mul.use ? dc->mul.col : 0xffffffff;

   if (smooth)
     evas_common_scale_rgba_smooth_draw(src, dst,
                                        clip_x, clip_y, clip_w, clip_h,
                                        mul_col, dc->render_op,
                                        src_x, src_y, src_w, src_h,
                                        dst_x, dst_y, dst_w, dst_h,
                                        dc->clip.mask, dc->clip.mask_x, dc->clip.mask_y);
   else
     evas_common_scale_rgba_sample_draw(src, dst,
                                        clip_x, clip_y, clip_w, clip_h,
                                        mul_col, dc->render_op,
                                        src_x, src_y, src_w, src_h,
                                        dst_x, dst_y, dst_w, dst_h,
                                        dc->clip.mask, dc->clip.mask_x, dc->clip.mask_y);
}

/**
 * @brief Callback wrapper for _map_image_draw with smooth=0.
 * Used with evas_common_scale_rgba_in_to_out_clip_cb.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return Always EINA_TRUE.
 */
static Eina_Bool
_map_image_sample_draw(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   _map_image_draw(src, dst, dc,
                   src_x, src_y, src_w, src_h,
                   dst_x, dst_y, dst_w, dst_h, 0);
   return EINA_TRUE;
}

/**
 * @brief Callback wrapper for _map_image_draw with smooth=1.
 * Used with evas_common_scale_rgba_in_to_out_clip_cb.
 * @param src Source image.
 * @param dst Destination image.
 * @param dc Drawing context.
 * @param src_x Source X.
 * @param src_y Source Y.
 * @param src_w Source W.
 * @param src_h Source H.
 * @param dst_x Destination X.
 * @param dst_y Destination Y.
 * @param dst_w Destination W.
 * @param dst_h Destination H.
 * @return Always EINA_TRUE.
 */
static Eina_Bool
_map_image_smooth_draw(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
   _map_image_draw(src, dst, dc,
                   src_x, src_y, src_w, src_h,
                   dst_x, dst_y, dst_w, dst_h, 1);
   return EINA_TRUE;
}

/**
 * @brief Executes an image map drawing command in a rendering thread.
 * Handles the optimization where a map operation is equivalent to a simple scale/blit.
 * Iterates through map points (typically in quads) and calls the appropriate drawing function.
 * This function is called by the thread pool. Frees associated resources afterwards.
 * @param data Pointer to an Evas_Thread_Command_Map structure containing drawing parameters.
 */
static void
_draw_thread_map_draw(void *data)
{
   Evas_Thread_Command_Map *map = data;
   int offset = map->offset;
   RGBA_Map *m = map->map;
   RGBA_Image *im = map->image;
   int dx, dy, dw, dh;

   do
     {
        if (m->count - offset < 4) goto free_out;

        //Fully Transparency. Skip this.
        if (!(m->pts[0 + offset].col & 0xff000000) &&
            !(m->pts[1 + offset].col & 0xff000000) &&
            !(m->pts[2 + offset].col & 0xff000000) &&
            !(m->pts[3 + offset].col & 0xff000000))
          {
             offset += 4;
             continue;
          }

        if (!(map->anti_alias && map->smooth) &&    //For sub-pixel rendering
            (m->pts[0 + offset].x == m->pts[3 + offset].x) &&
            (m->pts[1 + offset].x == m->pts[2 + offset].x) &&
            (m->pts[0 + offset].y == m->pts[1 + offset].y) &&
            (m->pts[3 + offset].y == m->pts[2 + offset].y) &&
            (m->pts[0 + offset].x <= m->pts[1 + offset].x) &&
            (m->pts[0 + offset].y <= m->pts[2 + offset].y) &&
            (m->pts[0 + offset].u == 0) &&
            (m->pts[0 + offset].v == 0) &&
            (m->pts[1 + offset].u == (int)(im->cache_entry.w << FP)) &&
            (m->pts[1 + offset].v == 0) &&
            (m->pts[2 + offset].u == (int)(im->cache_entry.w << FP)) &&
            (m->pts[2 + offset].v == (int)(im->cache_entry.h << FP)) &&
            (m->pts[3 + offset].u == 0) &&
            (m->pts[3 + offset].v == (int)(im->cache_entry.h << FP)) &&
            (m->pts[0].col == m->pts[1].col) &&
            (m->pts[1].col == m->pts[2].col) &&
            (m->pts[2].col == m->pts[3].col))
          {
             DATA32 col;
             Eina_Bool use;

             dx = m->pts[0 + offset].x >> FP;
             dy = m->pts[0 + offset].y >> FP;
             dw = (m->pts[2 + offset].x >> FP) - dx;
             dh = (m->pts[2 + offset].y >> FP) - dy;

             col = map->image_ctx->mul.col;
             use = map->image_ctx->mul.use;
             if (use) map->image_ctx->mul.col = MUL4_SYM(col, m->pts[0].col);
             else map->image_ctx->mul.col = m->pts[0].col;
             map->image_ctx->mul.use = 1;

             if (map->smooth)
               evas_common_scale_rgba_in_to_out_clip_cb
                 (im, map->surface, map->image_ctx,
                  0, 0, im->cache_entry.w, im->cache_entry.h,
                  dx, dy, dw, dh, _map_image_smooth_draw);
             else
               evas_common_scale_rgba_in_to_out_clip_cb
                 (im, map->surface, map->image_ctx,
                  0, 0, im->cache_entry.w, im->cache_entry.h,
                  dx, dy, dw, dh, _map_image_sample_draw);

             map->image_ctx->mul.col = col;
             map->image_ctx->mul.use = use;
          }
        else
          {
             evas_common_map_rgba_draw
               (im, map->surface,
                map->clip.x, map->clip.y, map->clip.w, map->clip.h,
                map->mul_col, map->render_op, m->count - offset, &m->pts[offset],
                map->smooth, map->anti_alias, map->level,
                map->mask, map->mask_x, map->mask_y);
          }

        evas_common_cpu_end_opt();

        offset += 4;
     }
   while ((m->count > 4) && (m->count - offset >= 4));

 free_out:
   free(m);
   evas_common_draw_context_free(map->image_ctx);
   eina_mempool_free(_mp_command_map, map);
}

/**
 * @brief Creates and enqueues an image map drawing command for threaded execution.
 * Duplicates the drawing context and the map data structure for the thread.
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The drawing context.
 * @param map The RGBA_Map structure defining the transformation.
 * @param smooth 1 for smooth rendering, 0 otherwise.
 * @param level Map rendering level (unused in software engine?).
 * @param offset Starting offset within the map points array (usually 0).
 * @return EINA_TRUE if the command was successfully enqueued, EINA_FALSE otherwise.
 */
static Eina_Bool
_map_draw_thread_cmd(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Map *map, int smooth, int level, int offset)
{
   Evas_Thread_Command_Map *cm;
   int clip_x, clip_y, clip_w, clip_h;

   cm = eina_mempool_malloc(_mp_command_map, sizeof (Evas_Thread_Command_Map));
   if (!cm) return EINA_FALSE;

   cm->image = src;
   cm->image_ctx = evas_common_draw_context_dup(dc);
   cm->surface = dst;

   if (dc->clip.use)
     {
	clip_x = dc->clip.x;
	clip_y = dc->clip.y;
	clip_w = dc->clip.w;
	clip_h = dc->clip.h;
     }
   else
     {
	clip_x = clip_y = 0;
	clip_w = dst->cache_entry.w;
	clip_h = dst->cache_entry.h;
     }

   EINA_RECTANGLE_SET(&cm->clip, clip_x, clip_y, clip_w, clip_h);

   cm->mul_col = dc->mul.use ? dc->mul.col : 0xffffffff;
   cm->render_op = dc->render_op;
   cm->anti_alias = dc->anti_alias;

   cm->map = calloc(1, sizeof(RGBA_Map) +
                    sizeof(RGBA_Map_Point) * map->count);
   cm->map->engine_data = map->engine_data;
   cm->map->image.w = map->image.w;
   cm->map->image.h = map->image.h;
   cm->map->uv.w = map->uv.w;
   cm->map->uv.h = map->uv.h;
   cm->map->x = map->x;
   cm->map->y = map->y;
   cm->map->count = map->count;
   memcpy(&cm->map->pts[0], &map->pts[0], sizeof(RGBA_Map_Point) * map->count);

   cm->smooth = smooth;
   cm->level = level;
   cm->offset = offset;

   cm->mask = dc->clip.mask;
   cm->mask_x = dc->clip.mask_x;
   cm->mask_y = dc->clip.mask_y;

   QCMD(_draw_thread_map_draw, cm);

   return EINA_TRUE;
}

/**
 * @brief Synchronous implementation for drawing an image map.
 * Handles the optimization where a map operation is equivalent to a simple scale/blit
 * by calling eng_image_draw directly. Otherwise, dispatches to pipe render or
 * common map drawing function. Recursively calls itself for maps with more than 4 points.
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (used for context manipulation).
 * @param context The drawing context.
 * @param surface The target surface (RGBA_Image).
 * @param im The source image (RGBA_Image).
 * @param m The RGBA_Map structure.
 * @param smooth Smooth rendering flag.
 * @param level Map rendering level.
 * @param offset Starting offset in the map points array.
 */
static void
evas_software_image_map_draw(void *engine EINA_UNUSED, void *data, void *context, RGBA_Image *surface, RGBA_Image *im, RGBA_Map *m, int smooth, int level, int offset)
{
   if (m->count - offset < 4) return;

   if ((m->pts[0 + offset].x == m->pts[3 + offset].x) &&
       (m->pts[1 + offset].x == m->pts[2 + offset].x) &&
       (m->pts[0 + offset].y == m->pts[1 + offset].y) &&
       (m->pts[3 + offset].y == m->pts[2 + offset].y) &&
       (m->pts[0 + offset].x <= m->pts[1 + offset].x) &&
       (m->pts[0 + offset].y <= m->pts[2 + offset].y) &&
       (m->pts[0 + offset].u == 0) &&
       (m->pts[0 + offset].v == 0) &&
       (m->pts[1 + offset].u == (int)(im->cache_entry.w << FP)) &&
       (m->pts[1 + offset].v == 0) &&
       (m->pts[2 + offset].u == (int)(im->cache_entry.w << FP)) &&
       (m->pts[2 + offset].v == (int)(im->cache_entry.h << FP)) &&
       (m->pts[3 + offset].u == 0) &&
       (m->pts[3 + offset].v == (int)(im->cache_entry.h << FP)) &&
       (m->pts[0 + offset].col == m->pts[1 + offset].col) &&
       (m->pts[1 + offset].col == m->pts[2 + offset].col) &&
       (m->pts[2 + offset].col == m->pts[3 + offset].col))
     {
        DATA32 col;
        int a, r, g, b;
        int dx, dy, dw, dh;
        int mul;

        mul = eng_context_multiplier_get(data, context, &r, &g, &b, &a);
        if (mul) col = MUL4_256(a, r, g, b, m->pts[0 + offset].col);
        else col = m->pts[0 + offset].col;
        eng_context_multiplier_set(data, context, R_VAL(&col), G_VAL(&col), B_VAL(&col), A_VAL(&col));

        dx = m->pts[0 + offset].x >> FP;
        dy = m->pts[0 + offset].y >> FP;
        dw = (m->pts[2 + offset].x >> FP) - dx;
        dh = (m->pts[2 + offset].y >> FP) - dy;
        eng_image_draw
          (engine, data, context, surface, im,
           0, 0, im->cache_entry.w, im->cache_entry.h,
           dx, dy, dw, dh, smooth,
           EINA_FALSE);

        if (mul) eng_context_multiplier_set(data, context, r, g, b, a);
        else eng_context_multiplier_unset(data, context);
     }
   else
     {
#ifdef BUILD_PIPE_RENDER
        if ((cpunum > 1))
	  {
             evas_common_pipe_map_draw(im, surface, context, m, smooth, level);
             return;
          }
        else
#endif
          {
             evas_common_map_rgba(im, surface, context, m->count - offset, &m->pts[offset], smooth, level);
          }
     }
   evas_common_cpu_end_opt();

   if (m->count > 4)
     {
        evas_software_image_map_draw(engine, data, context, surface, im, m, smooth, level, offset + 4);
     }
}

/**
 * @brief Engine function to draw a mapped image.
 * Dispatches the drawing to the appropriate implementation (sync or async thread).
 * Ensures source image data is loaded for async operations.
 * @param engine The engine instance (unused).
 * @param data Engine-specific data.
 * @param context The drawing context.
 * @param surface The target surface.
 * @param image The source image entry (Image_Entry).
 * @param m The RGBA_Map structure defining the transformation.
 * @param smooth 1 for smooth rendering, 0 otherwise.
 * @param level Map rendering level (unused in software engine?).
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 * @return EINA_TRUE if drawing was handled asynchronously, EINA_FALSE otherwise (sync or error).
 */
static Eina_Bool
eng_image_map_draw(void *engine EINA_UNUSED, void *data, void *context, void *surface, void *image, RGBA_Map *m, int smooth, int level, Eina_Bool do_async)
{
   RGBA_Image *im = image;

   if (!im) return EINA_FALSE;
   if (m->count < 3) return EINA_FALSE;

   if (do_async)
     {
        /* Since the thread that'll draw the map won't call eng_image_draw()
         * (which sends the load request of source image to Cserve2) - we need
         * to send the load request here before enqueueing thread command.
         */
        if (im->cache_entry.space == EVAS_COLORSPACE_ARGB8888)
          {
             evas_cache_image_load_data(&im->cache_entry);

             if (!im->cache_entry.flags.loaded) return EINA_FALSE;
          }

        return evas_common_map_thread_rgba_cb(im, surface, context,
                                              m, smooth, level, 0,
                                              _map_draw_thread_cmd);
     }
   else
     evas_software_image_map_draw(engine, data, context, surface, im, m,
                                  smooth, level, 0);

   return EINA_FALSE;
}

/**
 * @brief Cleans up resources associated with an RGBA_Map structure.
 * Currently frees engine-specific data if present.
 * @param data Engine-specific data (unused).
 * @param m The RGBA_Map structure to clean.
 */
static void
eng_image_map_clean(void *data EINA_UNUSED, RGBA_Map *m)
{
   evas_common_map_rgba_clean(m);
}

/**
 * @brief Creates a new surface suitable for image map operations.
 * Allocates an RGBA_Image with copied data (initially NULL data, effectively just allocating).
 * Ensures the pixel buffer is allocated.
 * @param data Engine-specific data (unused).
 * @param w Width of the surface.
 * @param h Height of the surface.
 * @param alpha Boolean indicating if the surface should support alpha.
 * @return A pointer to the newly created RGBA_Image surface, or NULL on failure.
 */
static void *
eng_image_map_surface_new(void *data EINA_UNUSED, int w, int h, int alpha)
{
   void *surface;

   surface = evas_cache_image_copied_data(evas_common_image_cache_get(),
                                          w, h, NULL, alpha,
                                          EVAS_COLORSPACE_ARGB8888);
   if (!surface) return NULL;
   evas_cache_image_pixels(surface);
   return surface;
}

/**
 * @brief Sets the scaling hint for an image.
 * Hints like static can allow caching of scaled versions.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry) to modify.
 * @param hint The Evas_Image_Scale_Hint value.
 */
static void
eng_image_scale_hint_set(void *data EINA_UNUSED, void *image, int hint)
{
   Image_Entry *im;

   if (!image) return;
   im = image;
   im->scale_hint = hint;
}

/**
 * @brief Gets the scaling hint for an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The Evas_Image_Scale_Hint value.
 */
static int
eng_image_scale_hint_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EVAS_IMAGE_SCALE_HINT_NONE;
   im = image;
   return im->scale_hint;
}

/**
 * @brief Checks if an image is animated.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return EINA_TRUE if the image is animated, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_image_animated_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EINA_FALSE;
   im = image;
   return im->animated.animated;
}

/**
 * @brief Gets the total number of frames in an animated image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The frame count, or -1 if the image is not animated.
 */
static int
eng_image_animated_frame_count_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return -1;
   im = image;
   if (!im->animated.animated) return -1;
   return im->animated.frame_count;
}

/**
 * @brief Gets the loop type hint for an animated image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The Evas_Image_Animated_Loop_Hint value.
 */
static Evas_Image_Animated_Loop_Hint
eng_image_animated_loop_type_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EVAS_IMAGE_ANIMATED_HINT_NONE;
   im = image;
   if (!im->animated.animated) return EVAS_IMAGE_ANIMATED_HINT_NONE;
   return im->animated.loop_hint;
}

/**
 * @brief Gets the loop count for an animated image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The number of times the animation should loop, or -1 if not animated.
 */
static int
eng_image_animated_loop_count_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return -1;
   im = image;
   if (!im->animated.animated) return -1;
   return im->animated.loop_count;
}

/**
 * @brief Gets the duration of a specific frame in an animated image.
 * May require loading frame information from the file.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @param start_frame The starting frame index (unused in current implementation?).
 * @param frame_num The index of the frame whose duration is requested.
 * @return The duration of the frame in seconds, or -1.0 on error or if not animated.
 */
static double
eng_image_animated_frame_duration_get(void *data EINA_UNUSED, void *image, int start_frame, int frame_num)
{
   Image_Entry *im;

   if (!image) return -1;
   im = image;
   if (!im->animated.animated) return -1;
   return evas_common_load_rgba_image_frame_duration_from_file(im, start_frame, frame_num);
}

/**
 * @brief Sets the current frame to be displayed for an animated image.
 * Updates the internal current frame index. Does not immediately load the frame data.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @param frame_index The index of the frame to set as current.
 * @return EINA_TRUE if the frame index was changed, EINA_FALSE otherwise (or if not animated).
 */
static Eina_Bool
eng_image_animated_frame_set(void *data EINA_UNUSED, void *image, int frame_index)
{
   Image_Entry *im;

   if (!image) return EINA_FALSE;
   im = image;
   if (!im->animated.animated) return EINA_FALSE;
   if (im->animated.cur_frame == frame_index) return EINA_FALSE;
   im->animated.cur_frame = frame_index;
   return EINA_TRUE;
}

/**
 * @brief Gets the index of the currently set frame for an animated image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The current frame index, or 0 if not animated.
 */
static int
eng_image_animated_frame_get(void *data EINA_UNUSED, void *image)
{
   Image_Entry *im;

   if (!image) return EINA_FALSE;
   im = image;
   if (!im->animated.animated) return EINA_FALSE;

   return im->animated.cur_frame;
}

/**
 * @brief Executes a multi-font drawing command in a rendering thread.
 * Iterates through the Evas_Font_Array, sets the color for each run, and draws the glyphs.
 * This function is called by the thread pool. Frees associated resources afterwards.
 * @param data Pointer to an Evas_Thread_Command_Multi_Font structure.
 */
static void
_draw_thread_multi_font_draw(void *data)
{
   Evas_Thread_Command_Multi_Font *mf = data;
   Evas_Font_Array_Data           *itr;

   EINA_INARRAY_FOREACH(mf->texts->array, itr)
     {
        unsigned int r, g, b, a;
        int x = mf->x + itr->x, y = mf->y;

        r = itr->color.r;
        g = itr->color.g;
        b = itr->color.b;
        a = itr->color.a;

        eng_context_color_set(NULL, mf->context, r, g, b, a);
        evas_common_font_draw(mf->surface, mf->context, x, y, itr->glyphs);
        evas_common_cpu_end_opt();
     }

   evas_common_draw_context_free(mf->context);
   eina_mempool_free(_mp_command_multi_font, mf);
}

/**
 * @brief Creates and enqueues a multi-font drawing command for threaded execution.
 * Duplicates the drawing context for the thread.
 * @param dst The destination RGBA_Image surface.
 * @param dc The drawing context.
 * @param x The base x-coordinate for drawing.
 * @param y The base y-coordinate for drawing.
 * @param texts The Evas_Font_Array containing text runs and glyphs.
 * @return EINA_TRUE if the command was successfully enqueued, EINA_FALSE otherwise.
 */
static Eina_Bool
_multi_font_draw_thread_cmd(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Font_Array *texts)
{
   Evas_Thread_Command_Multi_Font *mf;

   mf = eina_mempool_malloc(_mp_command_multi_font,
                            sizeof(Evas_Thread_Command_Multi_Font));
   if (!mf)
     {
        ERR("Failed to allocate memory on mempool for multiple text_props "
            "commands.");
        return EINA_FALSE;
     }

   mf->context = evas_common_draw_context_dup(dc);
   mf->surface = dst;
   mf->x = x;
   mf->y = y;
   mf->texts = texts;

   QCMD(_draw_thread_multi_font_draw, mf);

   return EINA_TRUE;
}

/**
 * @brief Engine function to draw multiple text runs defined by an Evas_Font_Array.
 * Currently only supports asynchronous (threaded) drawing.
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param font The font set (unused, information is in texts).
 * @param x The base x-coordinate.
 * @param y The base y-coordinate.
 * @param w Target width (unused).
 * @param h Target height (unused).
 * @param ow Output width (unused).
 * @param oh Output height (unused).
 * @param texts The Evas_Font_Array containing text runs, colors, and glyphs.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 * @return EINA_TRUE if drawing was handled asynchronously, EINA_FALSE otherwise (sync not implemented or error).
 */
static Eina_Bool
eng_multi_font_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, Evas_Font_Set *font EINA_UNUSED, int x, int y, int w EINA_UNUSED, int h EINA_UNUSED, int ow EINA_UNUSED, int oh EINA_UNUSED, Evas_Font_Array *texts, Eina_Bool do_async)
{
   if (!texts) return EINA_FALSE;

   if (do_async)
     return _multi_font_draw_thread_cmd(surface, context, x, y, texts);

   return EINA_FALSE;
}

/**
 * @brief Gets the alpha value of a pixel corresponding to a specific point on a potentially scaled/sub-regioned image.
 * Calculates the source pixel coordinate based on the destination coordinate and the source/destination regions.
 * Loads image data if necessary.
 * @param image The image entry (Image_Entry).
 * @param x The destination x-coordinate to query.
 * @param y The destination y-coordinate to query.
 * @param alpha Pointer to store the resulting alpha value (0-255).
 * @param src_region_x Source region X used for scaling.
 * @param src_region_y Source region Y used for scaling.
 * @param src_region_w Source region W used for scaling.
 * @param src_region_h Source region H used for scaling.
 * @param dst_region_x Destination region X where the source region is drawn.
 * @param dst_region_y Destination region Y where the source region is drawn.
 * @param dst_region_w Destination region W where the source region is drawn.
 * @param dst_region_h Destination region H where the source region is drawn.
 * @return EINA_TRUE if the alpha value was successfully retrieved (even if 0 due to OOB),
 *         EINA_FALSE if the image is invalid or data couldn't be loaded.
 */
static Eina_Bool
eng_pixel_alpha_get(void *image, int x, int y, DATA8 *alpha, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h)
{
   RGBA_Image *im = image;
   int px, py, dx, dy, sx, sy, src_w, src_h;
   double scale_w, scale_h;

   if (!im) return EINA_FALSE;

   if ((dst_region_x > x) || (x >= (dst_region_x + dst_region_w)) ||
       (dst_region_y > y) || (y >= (dst_region_y + dst_region_h)))
     {
        *alpha = 0;
        return EINA_FALSE;
     }

   src_w = im->cache_entry.w;
   src_h = im->cache_entry.h;
   if ((src_w == 0) || (src_h == 0))
     {
        *alpha = 0;
        return EINA_TRUE;
     }

   EINA_SAFETY_ON_TRUE_GOTO(src_region_x < 0, error_oob);
   EINA_SAFETY_ON_TRUE_GOTO(src_region_y < 0, error_oob);
   EINA_SAFETY_ON_TRUE_GOTO(src_region_x + src_region_w > src_w, error_oob);
   EINA_SAFETY_ON_TRUE_GOTO(src_region_y + src_region_h > src_h, error_oob);

   scale_w = (double)dst_region_w / (double)src_region_w;
   scale_h = (double)dst_region_h / (double)src_region_h;

   /* point at destination */
   dx = x - dst_region_x;
   dy = y - dst_region_y;

   /* point at source */
   sx = dx / scale_w;
   sy = dy / scale_h;

   /* pixel point (translated) */
   px = src_region_x + sx;
   py = src_region_y + sy;
   EINA_SAFETY_ON_TRUE_GOTO(px >= src_w, error_oob);
   EINA_SAFETY_ON_TRUE_GOTO(py >= src_h, error_oob);

   switch (im->cache_entry.space)
     {
     case EVAS_COLORSPACE_ARGB8888:
       {
          DATA32 *pixel;

          evas_cache_image_load_data(&im->cache_entry);

          if (!im->cache_entry.flags.loaded)
            {
               ERR("im %p has no pixels loaded yet", im);
               return EINA_FALSE;
            }

          pixel = im->image.data;
          pixel += ((py * src_w) + px);
          *alpha = ((*pixel) >> 24) & 0xff;
       }
       break;

     default:
        ERR("Colorspace %d not supported.", im->cache_entry.space);
        *alpha = 0;
     }

   return EINA_TRUE;

 error_oob:
   ERR("Invalid region src=(%d, %d, %d, %d), dst=(%d, %d, %d, %d), image=%dx%d",
       src_region_x, src_region_y, src_region_w, src_region_h,
       dst_region_x, dst_region_y, dst_region_w, dst_region_h,
       src_w, src_h);
   *alpha = 0;
   return EINA_TRUE;
}

/**
 * @brief Flushes the image cache.
 * Temporarily sets the cache size to 0, flushes common caches, then restores the size.
 * @param data Engine-specific data (unused).
 */
static void
eng_image_cache_flush(void *data EINA_UNUSED)
{
   int tmp_size;

   tmp_size = evas_common_image_get_cache();
   evas_common_image_set_cache(0);
   evas_common_rgba_image_scalecache_flush();
   evas_common_image_set_cache(tmp_size);
}

/**
 * @brief Sets the target size for the image cache.
 * Also updates the size of the RGBA image scale cache.
 * @param data Engine-specific data (unused).
 * @param bytes The target cache size in bytes.
 */
static void
eng_image_cache_set(void *data EINA_UNUSED, int bytes)
{
   evas_common_image_set_cache(bytes);
   evas_common_rgba_image_scalecache_size_set(bytes);
}

/**
 * @brief Gets the current target size of the image cache.
 * @param data Engine-specific data (unused).
 * @return The target cache size in bytes.
 */
static int
eng_image_cache_get(void *data EINA_UNUSED)
{
   return evas_common_image_get_cache();
}

/**
 * @brief Loads a font set (potentially multiple fonts for fallback) by name and size.
 * Uses the common font loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param name The primary font name.
 * @param size The font size.
 * @param wanted_rend Rendering flags (e.g., anti-aliasing).
 * @param bitmap_scalable Scalability hint for bitmap fonts.
 * @return An Evas_Font_Set handle (actually RGBA_Font*) on success, NULL on failure.
 */
static Evas_Font_Set *
eng_font_load(void *data EINA_UNUSED, const char *name, int size,
      Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   return (Evas_Font_Set *) evas_common_font_load(name, size, wanted_rend, bitmap_scalable);
}

/**
 * @brief Loads a font set from memory data.
 * Uses the common font loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param source Identifier for the memory source (e.g., "memory").
 * @param name The font name associated with this memory data.
 * @param size The font size.
 * @param fdata Pointer to the font data in memory.
 * @param fdata_size Size of the font data.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Scalability hint for bitmap fonts.
 * @return An Evas_Font_Set handle (actually RGBA_Font*) on success, NULL on failure.
 */
static Evas_Font_Set *
eng_font_memory_load(void *data EINA_UNUSED, const char *source, const char *name, int size, const void *fdata, int fdata_size,
                     Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   return (Evas_Font_Set *) evas_common_font_memory_load(source, name, size,
         fdata, fdata_size, wanted_rend, bitmap_scalable);
}

/**
 * @brief Adds a font (by name) to an existing font set for fallback purposes.
 * Uses the common font loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param font The existing Evas_Font_Set (RGBA_Font*) to add to.
 * @param name The name of the font to add.
 * @param size The size of the font to add.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Scalability hint for bitmap fonts.
 * @return The potentially updated Evas_Font_Set handle.
 */
static Evas_Font_Set *
eng_font_add(void *data EINA_UNUSED, Evas_Font_Set *font, const char *name, int size, Font_Rend_Flags wanted_rend,
             Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   return (Evas_Font_Set *) evas_common_font_add((RGBA_Font *) font, name,
         size, wanted_rend, bitmap_scalable);
}

/**
 * @brief Adds a font (from memory) to an existing font set for fallback purposes.
 * Uses the common font loading infrastructure.
 * @param data Engine-specific data (unused).
 * @param font The existing Evas_Font_Set (RGBA_Font*) to add to.
 * @param source Identifier for the memory source.
 * @param name The font name associated with this memory data.
 * @param size The font size.
 * @param fdata Pointer to the font data in memory.
 * @param fdata_size Size of the font data.
 * @param wanted_rend Rendering flags.
 * @param bitmap_scalable Scalability hint for bitmap fonts.
 * @return The potentially updated Evas_Font_Set handle.
 */
static Evas_Font_Set *
eng_font_memory_add(void *data EINA_UNUSED, Evas_Font_Set *font, const char *source, const char *name, int size, const void *fdata, int fdata_size,
                    Font_Rend_Flags wanted_rend, Efl_Text_Font_Bitmap_Scalable bitmap_scalable)
{
   return (Evas_Font_Set *) evas_common_font_memory_add((RGBA_Font *) font,
         source, name, size, fdata, fdata_size, wanted_rend, bitmap_scalable);
}

/**
 * @brief Frees a font set (decrements reference count).
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to free/unref.
 */
static void
eng_font_free(void *data EINA_UNUSED, Evas_Font_Set *font)
{
   evas_common_font_free((RGBA_Font *) font);
}

/**
 * @brief Gets the ascent of a font set (distance from baseline to top).
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to query.
 * @return The font ascent in pixels.
 */
static int
eng_font_ascent_get(void *data EINA_UNUSED, Evas_Font_Set *font)
{
   return evas_common_font_ascent_get((RGBA_Font *) font);
}

/**
 * @brief Gets the descent of a font set (distance from baseline to bottom).
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to query.
 * @return The font descent in pixels (usually a non-positive value).
 */
static int
eng_font_descent_get(void *data EINA_UNUSED, Evas_Font_Set *font)
{
   return evas_common_font_descent_get((RGBA_Font *) font);
}

/**
 * @brief Gets the maximum ascent of a font set over all its glyphs.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to query.
 * @return The maximum font ascent in pixels.
 */
static int
eng_font_max_ascent_get(void *data EINA_UNUSED, Evas_Font_Set *font)
{
   return evas_common_font_max_ascent_get((RGBA_Font *) font);
}

/**
 * @brief Gets the maximum descent of a font set over all its glyphs.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to query.
 * @return The maximum font descent in pixels (usually a non-positive value).
 */
static int
eng_font_max_descent_get(void *data EINA_UNUSED, Evas_Font_Set *font)
{
   return evas_common_font_max_descent_get((RGBA_Font *) font);
}

/**
 * @brief Calculates the bounding box size (width and height) for rendering given text properties.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to use for measurement.
 * @param text_props The text properties (including text string and layout info).
 * @param w Pointer to store the calculated width.
 * @param h Pointer to store the calculated height.
 */
static void
eng_font_string_size_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props, int *w, int *h)
{
   evas_common_font_query_size((RGBA_Font *) font, text_props, w, h);
}

/**
 * @brief Gets the horizontal inset (bearing) of the first glyph for the given text properties.
 * This is the horizontal distance from the drawing origin (pen position) to the left edge of the first glyph's bounding box.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to use for measurement.
 * @param text_props The text properties.
 * @return The horizontal inset in pixels.
 */
static int
eng_font_inset_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props)
{
   return evas_common_font_query_inset((RGBA_Font *) font, text_props);
}

/**
 * @brief Gets the horizontal right inset of the last glyph for the given text properties.
 * This is the horizontal distance from the right edge of the last glyph's bounding box to the final pen position after drawing the text.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to use for measurement.
 * @param text_props The text properties.
 * @return The horizontal right inset in pixels.
 */
static int
eng_font_right_inset_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props)
{
   return evas_common_font_query_right_inset((RGBA_Font *) font, text_props);
}

/**
 * @brief Gets the total horizontal advance for rendering the given text properties.
 * This is the distance the pen position moves horizontally after drawing the text.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to use for measurement.
 * @param text_props The text properties.
 * @return The horizontal advance in pixels.
 */
static int
eng_font_h_advance_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props)
{
   int h, v;

   evas_common_font_query_advance((RGBA_Font *) font, text_props, &h, &v);
   return h;
}

/**
 * @brief Gets the total vertical advance for rendering the given text properties.
 * This is the distance the pen position moves vertically after drawing the text (usually 0 for horizontal text).
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to use for measurement.
 * @param text_props The text properties.
 * @return The vertical advance in pixels.
 */
static int
eng_font_v_advance_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props)
{
   int h, v;

   evas_common_font_query_advance((RGBA_Font *) font, text_props, &h, &v);
   return v;
}

/**
 * @brief Gets the pen coordinates after drawing up to a specific character position.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) used for layout.
 * @param text_props The text properties.
 * @param pos The character position (index) in the text string.
 * @param cpen_x Pointer to store the horizontal pen position after the character at `pos`.
 * @param cy Pointer to store the vertical position (baseline) of the character at `pos`.
 * @param cadv Pointer to store the advance width of the character at `pos`.
 * @param ch Pointer to store the height of the character at `pos`.
 * @return The character index corresponding to the input position `pos` (can differ due to bidi).
 */
static int
eng_font_pen_coords_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props, int pos, int *cpen_x, int *cy, int *cadv, int *ch)
{
   return evas_common_font_query_pen_coords((RGBA_Font *) font, text_props, pos, cpen_x, cy, cadv, ch);
}

/**
 * @brief Populates an Evas_Text_Props structure with layout information for a given text string.
 * Performs BiDi analysis, script detection, and potentially itemization based on the mode.
 * @param data Engine-specific data (unused).
 * @param fi The specific font instance (RGBA_Font_Int*) to use (can be NULL).
 * @param text The Unicode text string.
 * @param text_props The Evas_Text_Props structure to populate.
 * @param par_props Pre-calculated BiDi paragraph properties (optional).
 * @param par_pos Starting position within the paragraph (for BiDi context).
 * @param len Length of the text segment to process.
 * @param mode The processing mode (e.g., BIDI_ONLY, FULL).
 * @param lang Language code (e.g., "en") for language-specific shaping.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
eng_font_text_props_info_create(void *data EINA_UNUSED, Evas_Font_Instance *fi, const Eina_Unicode *text, Evas_Text_Props *text_props, const Evas_BiDi_Paragraph_Props *par_props, size_t par_pos, size_t len, Evas_Text_Props_Mode mode, const char *lang)
{
   return evas_common_text_props_content_create((RGBA_Font_Int *) fi, text,
         text_props, par_props, par_pos, len, mode, lang);
}

/**
 * @brief Gets the geometry (bounding box) of the character at a specific position in the text.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) used for layout.
 * @param text_props The text properties.
 * @param pos The character position (index) in the text string.
 * @param cx Pointer to store the character's left x-coordinate relative to the text origin.
 * @param cy Pointer to store the character's top y-coordinate relative to the text origin.
 * @param cw Pointer to store the character's width.
 * @param ch Pointer to store the character's height.
 * @return The character index corresponding to the input position `pos` (can differ due to bidi).
 */
static int
eng_font_char_coords_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props, int pos, int *cx, int *cy, int *cw, int *ch)
{
   return evas_common_font_query_char_coords((RGBA_Font *) font, text_props, pos, cx, cy, cw, ch);
}

/**
 * @brief Finds the character index at given coordinates relative to the text origin.
 * Also returns the geometry of the found character.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) used for layout.
 * @param text_props The text properties.
 * @param x The x-coordinate relative to the text origin.
 * @param y The y-coordinate relative to the text origin.
 * @param cx Pointer to store the found character's left x-coordinate.
 * @param cy Pointer to store the found character's top y-coordinate.
 * @param cw Pointer to store the found character's width.
 * @param ch Pointer to store the found character's height.
 * @return The index of the character at the given coordinates, or -1 if no character is found there.
 */
static int
eng_font_char_at_coords_get(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props, int x, int y, int *cx, int *cy, int *cw, int *ch)
{
   return evas_common_font_query_char_at_coords((RGBA_Font *) font, text_props, x, y, cx, cy, cw, ch);
}

/**
 * @brief Finds the last character index that fits within a given horizontal coordinate range.
 * Useful for text truncation or line breaking calculations.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) used for layout.
 * @param text_props The text properties.
 * @param x The target x-coordinate relative to the text origin (usually the available width).
 * @param y The target y-coordinate (used to determine the line, often ignored for single line).
 * @param width_offset An offset added to the calculated width before comparing with x.
 * @return The index of the last character that fits within the coordinate `x`.
 */
static int
eng_font_last_up_to_pos(void *data EINA_UNUSED, Evas_Font_Set *font, const Evas_Text_Props *text_props, int x, int y, int width_offset)
{
   return evas_common_font_query_last_up_to_pos((RGBA_Font *) font, text_props, x, y, width_offset);
}

/**
 * @brief Determines the end of a text run based on font/script changes.
 * Finds how many characters starting from `text` can be rendered using the same
 * font instance (`cur_fi`) within the given script context.
 * @param data Engine-specific data (unused).
 * @param font The base Evas_Font_Set (RGBA_Font*).
 * @param script_fi Pointer to store the font instance appropriate for the `script`.
 * @param cur_fi Pointer to store the font instance used for the current run (might differ from script_fi due to fallback).
 * @param script The script type of the text run.
 * @param text Pointer to the beginning of the Unicode text run.
 * @param run_len The maximum length of the run to consider.
 * @return The number of characters from `text` that form a continuous run with the same font instance.
 */
static int
eng_font_run_font_end_get(void *data EINA_UNUSED, Evas_Font_Set *font, Evas_Font_Instance **script_fi, Evas_Font_Instance **cur_fi, Evas_Script_Type script, const Eina_Unicode *text, int run_len)
{
   return evas_common_font_query_run_font_end_get((RGBA_Font *) font,
         (RGBA_Font_Int **) script_fi, (RGBA_Font_Int **) cur_fi,
         script, text, run_len);
}

/**
 * @brief Executes a font (glyph array) drawing command in a rendering thread.
 * Sets up a temporary drawing context with necessary info from the command struct
 * and calls the common font drawing function.
 * This function is called by the thread pool. Frees associated resources afterwards.
 * @param data Pointer to an Evas_Thread_Command_Font structure containing drawing parameters.
 */
static void
_draw_thread_font_draw(void *data)
{
   Evas_Thread_Command_Font *font = data;
   RGBA_Draw_Context dc;
   memset(&dc, 0, sizeof(dc));

   dc.font_ext.data = font->font_ext_data;
   dc.font_ext.func.gl_new = font->gl_new;
   dc.font_ext.func.gl_free = font->gl_free;
   dc.font_ext.func.gl_draw = font->gl_draw;
   dc.col.col = font->col;
   dc.mul.col = font->mul_col;
   dc.mul.use = (font->mul_col == 0xffffffff) ? 0 : 1;
   dc.clip.use = font->clip_use;
   dc.clip.x = font->clip_rect.x;
   dc.clip.y = font->clip_rect.y;
   dc.clip.w = font->clip_rect.w;
   dc.clip.h = font->clip_rect.h;
   dc.clip.mask = font->mask;
   dc.clip.mask_x = font->mask_x;
   dc.clip.mask_y = font->mask_y;

   evas_common_font_rgba_draw
     (font->dst, &dc,
      font->x, font->y, font->glyphs, font->func,
      font->ext.x, font->ext.y, font->ext.w, font->ext.h,
      font->im_w, font->im_h);

   eina_mempool_free(_mp_command_font, font);
}

/**
 * @brief Creates and enqueues a font drawing command for threaded execution.
 * Populates the command structure with data from the drawing context and glyph array.
 * @param dst The destination RGBA_Image surface.
 * @param dc The drawing context.
 * @param x The x-coordinate for drawing the text.
 * @param y The y-coordinate for drawing the text (baseline).
 * @param glyphs The Evas_Glyph_Array containing glyphs and positions.
 * @param func The low-level glyph drawing function (e.g., evas_common_gfx_font_glyph_draw).
 * @param ext_x Clipping extent x.
 * @param ext_y Clipping extent y.
 * @param ext_w Clipping extent width.
 * @param ext_h Clipping extent height.
 * @param im_w Destination image width.
 * @param im_h Destination image height.
 * @return EINA_TRUE if the command was successfully enqueued, EINA_FALSE otherwise.
 */
static Eina_Bool
_font_draw_thread_cmd(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, Evas_Glyph_Array *glyphs, RGBA_Gfx_Func func, int ext_x, int ext_y, int ext_w, int ext_h, int im_w, int im_h)
{
   Evas_Thread_Command_Font *cf;

   cf = eina_mempool_malloc(_mp_command_font, sizeof (Evas_Thread_Command_Font));
   if (!cf) return EINA_FALSE;

   cf->dst = dst;
   cf->x = x;
   cf->y = y;
   cf->gl_new = dc->font_ext.func.gl_new;
   cf->gl_free = dc->font_ext.func.gl_free;
   cf->gl_draw = dc->font_ext.func.gl_draw;
   cf->font_ext_data = dc->font_ext.data;
   cf->col = dc->col.col;
   cf->mul_col = dc->mul.use ? dc->mul.col : 0xffffffff;
   cf->clip_use = dc->clip.use;
   EINA_RECTANGLE_SET(&cf->clip_rect,
                      dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
   cf->glyphs = glyphs;
   cf->func = func;
   EINA_RECTANGLE_SET(&cf->ext, ext_x, ext_y, ext_w, ext_h);
   cf->im_w = im_w;
   cf->im_h = im_h;
   cf->mask = dc->clip.mask;
   cf->mask_x = dc->clip.mask_x;
   cf->mask_y = dc->clip.mask_y;

   QCMD(_draw_thread_font_draw, cf);

   return EINA_TRUE;
}

/**
 * @brief Engine function to draw text based on text properties.
 * Prepares the glyphs if needed, then dispatches the drawing to the appropriate
 * implementation (sync, async thread, pipe).
 * @param engine The engine instance (unused).
 * @param data Engine-specific data (unused).
 * @param context The drawing context.
 * @param surface The target surface.
 * @param font The font set (unused, information is in text_props).
 * @param x The x-coordinate for drawing the text.
 * @param y The y-coordinate for drawing the text (baseline).
 * @param w Target width (unused).
 * @param h Target height (unused).
 * @param ow Output width (unused).
 * @param oh Output height (unused).
 * @param text_props The Evas_Text_Props containing the text, layout, and glyph information.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 * @return EINA_TRUE if drawing was handled asynchronously, EINA_FALSE otherwise (sync or error).
 */
static Eina_Bool
eng_font_draw(void *engine EINA_UNUSED, void *data EINA_UNUSED, void *context, void *surface, Evas_Font_Set *font EINA_UNUSED, int x, int y, int w EINA_UNUSED, int h EINA_UNUSED, int ow EINA_UNUSED, int oh EINA_UNUSED, Evas_Text_Props *text_props, Eina_Bool do_async)
{
   if (do_async)
     {
        evas_common_font_draw_prepare(text_props);
        if (!text_props->glyphs) return EINA_FALSE;

        return evas_common_font_draw_cb(surface, context, x, y, text_props->glyphs,
                                        _font_draw_thread_cmd);
     }
#ifdef BUILD_PIPE_RENDER
   else if ((cpunum > 1))
     evas_common_pipe_text_draw(surface, context, x, y, text_props);
#endif
   else
     {
        evas_common_font_draw_prepare(text_props);
        evas_common_font_draw(surface, context, x, y, text_props->glyphs);
        evas_common_cpu_end_opt();
     }

   return EINA_FALSE;
}

/**
 * @brief Flushes the font cache.
 * Temporarily sets the cache size to 0, flushes common font caches, then restores the size.
 * @param data Engine-specific data (unused).
 */
static void
eng_font_cache_flush(void *data EINA_UNUSED)
{
   int tmp_size;

   tmp_size = evas_common_font_cache_get();
   evas_common_font_cache_set(0);
   evas_common_font_flush();
   evas_common_font_cache_set(tmp_size);
}

/**
 * @brief Sets the target size for the font cache.
 * @param data Engine-specific data (unused).
 * @param bytes The target cache size in bytes.
 */
static void
eng_font_cache_set(void *data EINA_UNUSED, int bytes)
{
   evas_common_font_cache_set(bytes);
}

/**
 * @brief Gets the current target size of the font cache.
 * @param data Engine-specific data (unused).
 * @return The target cache size in bytes.
 */
static int
eng_font_cache_get(void *data EINA_UNUSED)
{
   return evas_common_font_cache_get();
}

/**
 * @brief Sets the hinting mode for a font set.
 * @param data Engine-specific data (unused).
 * @param font The Evas_Font_Set (RGBA_Font*) to modify.
 * @param hinting The Evas_Font_Hinting_Flags value.
 */
static void
eng_font_hinting_set(void *data EINA_UNUSED, Evas_Font_Set *font, int hinting)
{
   evas_common_font_hinting_set((RGBA_Font *) font, hinting);
}

/**
 * @brief Checks if a specific hinting mode is available/supported by the underlying font backend (FreeType).
 * @param data Engine-specific data (unused).
 * @param hinting The Evas_Font_Hinting_Flags value to check.
 * @return 1 if the hinting mode is available, 0 otherwise.
 */
static int
eng_font_hinting_can_hint(void *data EINA_UNUSED, int hinting)
{
   return evas_common_hinting_available(hinting);
}

/**
 * @brief Checks if the engine's output supports alpha blending.
 * The software engine always supports alpha.
 * @param data Engine-specific data (unused).
 * @return Always EINA_TRUE.
 */
static Eina_Bool
eng_canvas_alpha_get(void *data EINA_UNUSED)
{
   return EINA_TRUE;
}


/**
 * @brief Gets the last load error code associated with an image.
 * @param data Engine-specific data (unused).
 * @param image The image entry (Image_Entry).
 * @return The Evas_Load_Error code from the last load attempt.
 */
static int
eng_image_load_error_get(void *data EINA_UNUSED, void *image)
{
   RGBA_Image *im;

   if (!image) return EVAS_LOAD_ERROR_NONE;
   im = image;
   return im->cache_entry.load_error;
}


//------------ Evas GL engine code ---------------//
#ifdef EVAS_GL
/**
 * @brief Initializes thread-local storage (TLS) keys for current GL context and surface.
 * Ensures TLS is initialized only once. Not thread-safe itself, relies on external locking if needed.
 * @return 1 on success, 0 on failure to create TLS keys.
 */
static inline int
_tls_check(void)
{
   // note: this is not thread safe...
   if (!_tls_init)
     {
        if (!eina_tls_new(&gl_current_ctx_key)) return 0;
        if (!eina_tls_new(&gl_current_sfc_key)) return 0;
        eina_tls_set(gl_current_ctx_key, NULL);
        eina_tls_set(gl_current_sfc_key, NULL);
     }
   _tls_init = EINA_TRUE;
   return 1;
}
#endif

/**
 * @brief Checks if the GL library (OSMesa) is initialized and available.
 * Calls gl_lib_init() if not already initialized.
 * @return EINA_TRUE if GL is available, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_check_gl(void)
{
   if (!gl_lib_init()) return 0;
   return 1;
}

/**
 * @brief Checks if the engine supports Evas GL (via OSMesa).
 * @param data Engine-specific data (unused).
 * @return EINA_TRUE if OSMesa is available, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_gl_supports_evas_gl(void *data EINA_UNUSED)
{
   return _check_gl();
}

/**
 * @brief Creates an off-screen GL rendering surface (using OSMesa).
 * Allocates memory for the pixel buffer based on configuration.
 * @param data Engine-specific data (unused).
 * @param config Pointer to an Evas_GL_Config structure specifying format, depth, stencil bits.
 * @param w Width of the surface.
 * @param h Height of the surface.
 * @return A pointer to the Render_Engine_GL_Surface structure on success, NULL on failure.
 */
static void *
eng_gl_surface_create(void *data EINA_UNUSED, void *config, int w, int h)
{
#ifdef EVAS_GL
   Render_Engine_GL_Surface *sfc;
   Evas_GL_Config *cfg;

   if (!_check_gl()) return NULL;

   sfc = calloc(1, sizeof(Render_Engine_GL_Surface));
   if (!sfc) return NULL;

   cfg = (Evas_GL_Config *)config;

   sfc->initialized  = 0;
   sfc->w            = w;
   sfc->h            = h;

   // Color Format
   switch (cfg->color_format)
     {

      case EVAS_GL_RGB_888:
         sfc->internal_fmt = OSMESA_BGRA;
         sfc->internal_cpp = 4;
// FIXME: don't allow rgb buffers as evas doesn't understand them so pad out
// to 32bit bgra buffers anyway, so for now just pad out - one day do this
// nicely
//         sfc->internal_fmt = OSMESA_RGB;
//         sfc->internal_cpp = 3;
         break;
      case EVAS_GL_RGBA_8888:
         sfc->internal_fmt = OSMESA_BGRA;
         sfc->internal_cpp = 4;
         break;
      default:
         sfc->internal_fmt = OSMESA_RGBA;
         sfc->internal_cpp = 4;
         break;
     }

   // Depth Bits
   switch (cfg->depth_bits)
     {
      case EVAS_GL_DEPTH_BIT_8:
         sfc->depth_bits = 8;
         break;
      case EVAS_GL_DEPTH_BIT_16:
         sfc->depth_bits = 16;
         break;
      case EVAS_GL_DEPTH_BIT_24:
         sfc->depth_bits = 24;
         break;
      case EVAS_GL_DEPTH_BIT_32:
         sfc->depth_bits = 32;
         break;
      case EVAS_GL_DEPTH_NONE:
      default:
         sfc->depth_bits = 0;
         break;
     }

   // Stencil Bits
   switch (cfg->stencil_bits)
     {
      case EVAS_GL_STENCIL_BIT_1:
         sfc->stencil_bits = 1;
         break;
      case EVAS_GL_STENCIL_BIT_2:
         sfc->stencil_bits = 2;
         break;
      case EVAS_GL_STENCIL_BIT_4:
         sfc->stencil_bits = 4;
         break;
      case EVAS_GL_STENCIL_BIT_8:
         sfc->stencil_bits = 8;
         break;
      case EVAS_GL_STENCIL_BIT_16:
         sfc->stencil_bits = 16;
         break;
      case EVAS_GL_STENCIL_NONE:
      default:
         sfc->stencil_bits = 0;
         break;
     }

   sfc->buffer = malloc(sizeof(unsigned char)*sfc->internal_cpp*w*h);

   if (!sfc->buffer)
     {
        free(sfc);
        return NULL;
     }

   return sfc;
#else
   (void) config;
   (void) w;
   (void) h;
   return NULL;
#endif
}

/**
 * @brief Destroys an off-screen GL rendering surface.
 * Frees the associated pixel buffer and the surface structure.
 * Unsets the current surface in TLS if this surface was current.
 * @param data Engine-specific data (unused).
 * @param surface Pointer to the Render_Engine_GL_Surface to destroy.
 * @return 1 on success, 0 if GL is not available or surface is NULL.
 */
static int
eng_gl_surface_destroy(void *data EINA_UNUSED, void *surface)
{
#ifdef EVAS_GL
   Render_Engine_GL_Surface *sfc;

   if (!_check_gl()) return 0;

   sfc = (Render_Engine_GL_Surface*)surface;

   if (!sfc) return 0;

   _tls_check();
   if (sfc == eina_tls_get(gl_current_sfc_key))
     eina_tls_set(gl_current_sfc_key, NULL);

   if (sfc->buffer) free(sfc->buffer);

   free(sfc);

   surface = NULL;

   return 1;
#else
   (void) surface;
   return 1;
#endif
}

/**
 * @brief Creates a GL rendering context (using OSMesa).
 * Currently only supports GLES 2.0 contexts. The actual OSMesa context
 * is created lazily during the first make_current call.
 * @param data Engine-specific data (unused).
 * @param share_context Optional pointer to another Render_Engine_GL_Context to share resources with.
 * @param version The requested GL version (must be EVAS_GL_GLES_2_X).
 * @param native_context_get Function pointer (unused in this engine).
 * @param engine_data_get Function pointer (unused in this engine).
 * @return A pointer to the Render_Engine_GL_Context structure on success, NULL on failure or unsupported version.
 */
static void *
eng_gl_context_create(void *data EINA_UNUSED, void *share_context, int version,
                      void *(*native_context_get)(void *) EINA_UNUSED,
                      void *(*engine_data_get)(void *) EINA_UNUSED)
{
#ifdef EVAS_GL
   Render_Engine_GL_Context *ctx;
   Render_Engine_GL_Context *share_ctx;

   if (!_check_gl()) return NULL;

   if (version != EVAS_GL_GLES_2_X)
     {
        ERR("This engine only supports OpenGL-ES 2.0 contexts for now!");
        return NULL;
     }

   ctx = calloc(1, sizeof(Render_Engine_GL_Context));

   if (!ctx) return NULL;

   share_ctx = (Render_Engine_GL_Context *)share_context;

   ctx->share_ctx = share_ctx;

#if 0
   if (share_ctx)
      ctx->context = _sym_OSMesaCreateContextExt( OSMESA_RGBA, 8, 0, 0, share_ctx->context );
   else
      ctx->context = _sym_OSMesaCreateContextExt( OSMESA_RGBA, 8, 0, 0, NULL );


   if (!ctx->context)
     {
        ERR("Error creating OSMesa Context.");
        free(ctx);
        return NULL;
     }
#endif

   ctx->initialized = 0;

   return ctx;
#else
   (void) share_context;
   return NULL;
#endif
}

/**
 * @brief Destroys a GL rendering context.
 * Destroys the underlying OSMesa context and frees the context structure.
 * Unsets the current context in TLS if this context was current.
 * @param data Engine-specific data (unused).
 * @param context Pointer to the Render_Engine_GL_Context to destroy.
 * @return 1 on success, 0 if GL is not available or context is NULL.
 */
static int
eng_gl_context_destroy(void *data EINA_UNUSED, void *context)
{
#ifdef EVAS_GL
   Render_Engine_GL_Context *ctx;

   if (!_check_gl()) return 0;

   ctx = (Render_Engine_GL_Context*)context;

   if (!ctx) return 0;

   _tls_check();
   if (ctx == eina_tls_get(gl_current_ctx_key))
     eina_tls_set(gl_current_ctx_key, NULL);

   _sym_OSMesaDestroyContext(ctx->context);

   free(ctx);
   context = NULL;

   return 1;
#else
   (void) context;
   return 0;
#endif
}

/**
 * @brief Makes a GL context current with a specific surface.
 * If the context hasn't been initialized yet, creates the OSMesa context now,
 * potentially sharing with another context if specified during creation.
 * Calls OSMesaMakeCurrent and sets the current context/surface in TLS.
 * Supports unsetting the current context/surface by passing NULL for both.
 * @param data Engine-specific data (unused).
 * @param surface Pointer to the Render_Engine_GL_Surface (or NULL).
 * @param context Pointer to the Render_Engine_GL_Context (or NULL).
 * @return 1 on success, 0 on failure (GL unavailable, OSMesa error, mismatched NULLs).
 */
static int
eng_gl_make_current(void *data EINA_UNUSED, void *surface, void *context)
{
#ifdef EVAS_GL
   Render_Engine_GL_Surface *sfc;
   Render_Engine_GL_Context *ctx;
   OSMesaContext share_ctx;
   GLboolean ret;

   if (!_check_gl()) return 0;

   sfc = (Render_Engine_GL_Surface*)surface;
   ctx = (Render_Engine_GL_Context*)context;

   _tls_check();

   if ((!sfc) ^ (!ctx))
     {
        ERR("Evas GL on SW engine does not support surfaceless contexts.");
        return 0;
     }

   // Unset surface/context
   if ((!sfc) && (!ctx))
     {
        eina_tls_set(gl_current_ctx_key, NULL);
        eina_tls_set(gl_current_sfc_key, NULL);
        return 1;
     }

   // Initialize Context if it hasn't been.
   if (!ctx->initialized)
     {
        if (ctx->share_ctx)
          share_ctx = ctx->share_ctx->context;
        else
          share_ctx = NULL;

        ctx->context = _sym_OSMesaCreateContextExt(sfc->internal_fmt,
                                                   sfc->depth_bits,
                                                   sfc->stencil_bits,
                                                   0,
                                                   share_ctx);
        if (!ctx->context)
          {
             ERR("Error initializing context.");
             eina_tls_set(gl_current_ctx_key, NULL);
             eina_tls_set(gl_current_sfc_key, NULL);
             return 0;
          }

        ctx->initialized = 1;
     }


   // Call MakeCurrent
   ret = _sym_OSMesaMakeCurrent(ctx->context, sfc->buffer, GL_UNSIGNED_BYTE,
                                sfc->w, sfc->h);

   if (ret == GL_FALSE)
     {
        ERR("Error doing MakeCurrent.");
        eina_tls_set(gl_current_ctx_key, NULL);
        eina_tls_set(gl_current_sfc_key, NULL);
        return 0;
     }

   _sym_OSMesaPixelStore(OSMESA_Y_UP, 0);

   // Set the current surface/context
   ctx->current_sfc = sfc;
   sfc->current_ctx = ctx;
   eina_tls_set(gl_current_ctx_key, ctx);
   eina_tls_set(gl_current_sfc_key, sfc);

   return 1;
#else
   (void) surface;
   (void) context;
   return 1;
#endif
}

/**
 * @brief Queries GL implementation strings (e.g., GL_VERSION, GL_VENDOR).
 * Currently not implemented for the software engine.
 * @param data Engine-specific data (unused).
 * @param name The GLenum specifying the string to query (unused).
 * @return Always returns NULL.
 */
// FIXME!!! Implement later
static const char *
eng_gl_string_query(void *data EINA_UNUSED, int name EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Gets the address of a GL extension function.
 * Uses OSMesaGetProcAddress if available, otherwise falls back to dlsym.
 * @param data Engine-specific data (unused).
 * @param name The name of the GL function.
 * @return A pointer to the function address, or NULL if not found or GL is unavailable.
 */
static void *
eng_gl_proc_address_get(void *data EINA_UNUSED, const char *name)
{
#ifdef EVAS_GL
   if (!_check_gl()) return NULL;

   if (_sym_OSMesaGetProcAddress) return _sym_OSMesaGetProcAddress(name);
   return dlsym(RTLD_DEFAULT, name);
#else
   (void) name;
   return NULL;
#endif
}

/**
 * @brief Fills an Evas_Native_Surface structure with information about a GL surface.
 * Allows retrieving the underlying pixel buffer pointer for use elsewhere.
 * @param data Engine-specific data (unused).
 * @param surface Pointer to the Render_Engine_GL_Surface.
 * @param native_surface Pointer to an Evas_Native_Surface structure to be filled.
 * @return 1 on success, 0 if GL is unavailable or surface is NULL.
 */
static int
eng_gl_native_surface_get(void *data EINA_UNUSED, void *surface, void *native_surface)
{
#ifdef EVAS_GL
   Render_Engine_GL_Surface *sfc;
   Evas_Native_Surface *ns;

   if (!_check_gl()) return 0;

   sfc = (Render_Engine_GL_Surface*)surface;
   ns  = (Evas_Native_Surface*)native_surface;

   if (!sfc) return 0;

   ns->type = EVAS_NATIVE_SURFACE_EVASGL;
   ns->version = EVAS_NATIVE_SURFACE_VERSION;
   ns->data.evasgl.surface = sfc->buffer;

   return 1;
#else
   (void) surface;
   (void) native_surface;
   return 1;
#endif
}

/**
 * @brief Gets the Evas_GL_API function table for a specific GL version.
 * Currently only supports GLES 2.0. Initializes GL symbols if necessary.
 * @param data Engine-specific data (unused).
 * @param version The requested GL version (must be EVAS_GL_GLES_2_X).
 * @return A pointer to the Evas_GL_API structure, or NULL if version is unsupported or GL is unavailable.
 */
static void *
eng_gl_api_get(void *data EINA_UNUSED, int version)
{
   if (version != EVAS_GL_GLES_2_X)
     return NULL;

#ifdef EVAS_GL
   if (!_check_gl()) return NULL;

   return &gl_funcs;
#else
   return NULL;
#endif
}

/**
 * @brief Gets the last Evas GL error code for the engine.
 * Currently only checks if the output buffer exists.
 * @param data Pointer to the Render_Output_Software_Generic structure.
 * @return An Evas_GL_Error code (e.g., EVAS_GL_SUCCESS, EVAS_GL_BAD_DISPLAY).
 */
static int
eng_gl_error_get(void *data)
{
   Render_Output_Software_Generic *re = data;

   // TODO: Track EGL-like errors in the software engines

   if (!re->ob)
     return EVAS_GL_BAD_DISPLAY;

   return EVAS_GL_SUCCESS;
}

/**
 * @brief Gets the currently active GL context for the calling thread.
 * Retrieves the context pointer from thread-local storage.
 * @param data Engine-specific data (unused).
 * @return Pointer to the current Render_Engine_GL_Context, or NULL if none is current.
 */
static void *
eng_gl_current_context_get(void *data EINA_UNUSED)
{
   _tls_check();
   return eina_tls_get(gl_current_ctx_key);
}

/**
 * @brief Gets the currently active GL surface for the calling thread.
 * Retrieves the surface pointer from thread-local storage.
 * @param data Engine-specific data (unused).
 * @return Pointer to the current Render_Engine_GL_Surface, or NULL if none is current.
 */
static void *
eng_gl_current_surface_get(void *data EINA_UNUSED)
{
   _tls_check();
   return eina_tls_get(gl_current_sfc_key);
}

/**
 * @brief Gets the rotation angle applied to the GL output.
 * The software engine does not support rotation at the GL level.
 * @param data Engine-specific data (unused).
 * @return Always returns 0.
 */
static int
eng_gl_rotation_angle_get(void *data EINA_UNUSED)
{
   return 0;
}

//------------------------------------------------//

/* The following function require that any engine
   inheriting from software generic to have at the
   top of their render engine structure a
   Render_Output_Software_Generic structure that is
   initialized by evas_render_engine_software_generic_init().
 */

/**
 * @brief Creates a new instance of the software generic rendering engine.
 * Allocates the main engine structure and initializes the Ector surface cache.
 * @return Pointer to the new Render_Engine_Software_Generic instance, or NULL on failure.
 */
static void *
eng_engine_new(void)
{
   Render_Engine_Software_Generic *engine;

   engine = calloc(1, sizeof (Render_Engine_Software_Generic));
   if (!engine) return NULL;

   engine->surface_cache = generic_cache_new(engine, eng_image_free);

   return engine;
}

/**
 * @brief Frees an instance of the software generic rendering engine.
 * Destroys the Ector surface cache, checks for leaked outputs, and frees the engine structure.
 * @param engine Pointer to the Render_Engine_Software_Generic instance to free.
 */
static void
eng_engine_free(void *engine)
{
   Render_Engine_Software_Generic *e = engine;
   Render_Output_Software_Generic *output;

   generic_cache_destroy(e->surface_cache);

   EINA_LIST_FREE(e->outputs, output)
     ERR("Output %p not properly cleaned before engine destruction.", output);

   free(e);
}

/**
 * @brief Resizes the output buffer and associated tile buffer for a specific output.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 * @param w The new width.
 * @param h The new height.
 */
static void
eng_output_resize(void *engine EINA_UNUSED, void *data, int w, int h)
{
   Render_Output_Software_Generic *re;

   re = (Render_Output_Software_Generic *)data;
   re->outbuf_reconfigure(re->ob, w, h, re->outbuf_get_rot(re->ob),
                          OUTBUF_DEPTH_INHERIT);
   evas_common_tilebuf_free(re->tb);
   re->tb = evas_common_tilebuf_new(w, h);
   if (re->tb)
     {
        evas_common_tilebuf_set_tile_size(re->tb, TILESIZE, TILESIZE);
        evas_common_tilebuf_tile_strict_set(re->tb, re->tile_strict);
     }
   re->w = w;
   re->h = h;
}

/**
 * @brief Adds a rectangle to the redraw list for all outputs managed by the engine.
 * @param engine Pointer to the Render_Engine_Software_Generic instance.
 * @param x The x-coordinate of the redraw rectangle.
 * @param y The y-coordinate of the redraw rectangle.
 * @param w The width of the redraw rectangle.
 * @param h The height of the redraw rectangle.
 */
static void
eng_output_redraws_rect_add(void *engine, int x, int y, int w, int h)
{
   Render_Engine_Software_Generic *backend = engine;
   Render_Output_Software_Generic *re;
   Eina_List *l;

   EINA_LIST_FOREACH(backend->outputs, l, re)
     evas_common_tilebuf_add_redraw(re->tb, x, y, w, h);
}

/**
 * @brief Deletes/subtracts a rectangle from the redraw list for all outputs.
 * @param engine Pointer to the Render_Engine_Software_Generic instance.
 * @param x The x-coordinate of the rectangle to remove.
 * @param y The y-coordinate of the rectangle to remove.
 * @param w The width of the rectangle to remove.
 * @param h The height of the rectangle to remove.
 */
static void
eng_output_redraws_rect_del(void *engine, int x, int y, int w, int h)
{
   Render_Engine_Software_Generic *backend = engine;
   Render_Output_Software_Generic *re;
   Eina_List *l;

   EINA_LIST_FOREACH(backend->outputs, l, re)
     evas_common_tilebuf_del_redraw(re->tb, x, y, w, h);
}

/**
 * @brief Clears the redraw list for a specific output.
 * Also calls the output buffer's clear function if available.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 */
static void
eng_output_redraws_clear(void *engine EINA_UNUSED, void *data)
{
   Render_Output_Software_Generic *re;

   re = (Render_Output_Software_Generic *)data;
   evas_common_tilebuf_clear(re->tb);
   if (re->outbuf_redraws_clear) re->outbuf_redraws_clear(re->ob);
}

/**
 * @brief Merges redraw rectangles using a "smart" algorithm.
 * Tries to combine nearby rectangles into larger bounding boxes if the
 * resulting area increase is within a certain percentage threshold, aiming
 * to reduce the number of separate update regions while not excessively
 * increasing the total pixel area to redraw. Also merges vertically adjacent
 * rectangles with the same width.
 * @param tb The Tilebuf (used for width/height).
 * @param rects The initial list of redraw rectangles (Tilebuf_Rect list). This list is freed by the function.
 * @return A new list of merged redraw rectangles (Tilebuf_Rect list).
 */
static Tilebuf_Rect *
_smart_merge(Tilebuf *tb, Tilebuf_Rect *rects)
{
   Tilebuf_Rect *merged, *mergelist = NULL, *r;
   Box *box;
   Region *region;
   int i, j, k, n, n2;
   Eina_Bool did_merge;

   n = eina_inlist_count(EINA_INLIST_GET(rects));
   box = malloc(n * sizeof(Box));
   i = 0;
   EINA_INLIST_FOREACH(EINA_INLIST_GET(rects), r)
     {
        box[i].x1 = r->x;
        box[i].y1 = r->y;
        box[i].x2 = r->x + r->w;
        box[i].y2 = r->y + r->h;
        i++;
     }
   evas_common_tilebuf_free_render_rects(rects);

   n2 = n;
   for (;;)
     {
        Box *box2, bbox;
        int mergenum, area, minarea, area1, area2, perc;

        did_merge = EINA_FALSE;
        box2 = calloc(1, n2 * sizeof(Box));
        j = 0;
        for (i = 0; i < n2; i++)
          {
             if (box[i].x1 == box[i].x2) continue;

             mergenum = -1;
             minarea = 0x7fffffff;

             box2[j] = box[i];
             area1 = (box[i].x2 - box[i].x1) * (box[i].y2 - box[i].y1);
             box[i].x1 = 0;
             box[i].x2 = 0;
             for (k = i + 1; k < n; k++)
               {
                  if (box[k].x1 == box[k].x2) continue;
                  bbox = box2[j];
                  if (box[k].x1 < bbox.x1) bbox.x1 = box[k].x1;
                  if (box[k].y1 < bbox.y1) bbox.y1 = box[k].y1;
                  if (box[k].x2 > bbox.x2) bbox.x2 = box[k].x2;
                  if (box[k].y2 > bbox.y2) bbox.y2 = box[k].y2;
                  area = (bbox.x2 - bbox.x1) * (bbox.y2 - bbox.y1);
                  if (area < minarea)
                    {
                       mergenum = k;
                       minarea = area;
                    }
               }
             if (mergenum >= 0)
               {
                  k = mergenum;
                  area2 = (box[k].x2 - box[k].x1) * (box[k].y2 - box[k].y1);
                  perc = (minarea * 100) / (area1 + area2);
                  // if combined size of bounding box of rects is <= X% of
                  // the sum of the 2 src areas - then merge
                  if (perc <= 150)
                    {
                       bbox = box2[j];
                       if (box[k].x1 < bbox.x1) bbox.x1 = box[k].x1;
                       if (box[k].y1 < bbox.y1) bbox.y1 = box[k].y1;
                       if (box[k].x2 > bbox.x2) bbox.x2 = box[k].x2;
                       if (box[k].y2 > bbox.y2) bbox.y2 = box[k].y2;
                       box2[j] = bbox;
                       box[k].x1 = 0;
                       box[k].x2 = 0;
                       did_merge = EINA_TRUE;
                    }
               }
             j++;
          }
        free(box);
        box = box2;
        if (!did_merge) break;
     }
   region = region_new(tb->outbuf_w, tb->outbuf_h);
   for (i = 0; i < n2; i++)
     {
        if (box[i].x1 == box[i].x2) continue;
        region_rect_add(region,
                        box[i].x1, box[i].y1,
                        box[i].x2 - box[i].x1,
                        box[i].y2 - box[i].y1);
     }
   free(box);
   box = region_rects(region);
   n = region_rects_num(region);
   merged = calloc(1, n * sizeof(Tilebuf_Rect));
   if (merged) {
        j = 0;
#if 1
        // regions sometimes produce box sets like:
        // +---+
        // |   |
        // +---+    +-------+
        // |   |    |       |
        // +---+    +-------+---------+
        //          |                 |
        //          |                 |
        //          +-----------------+
        // so the upper-left 2 boxes can be merged into 1 and they have the same
        // x coords and are flush-aligned one above the other. that is what
        // this does - find these and merge them to have fewer rects
        for (i = 0; i < n; i++)
          {
             // skip empty boxes
             if (box[i].x1 == box[i].x2) continue;
             // walk all following boxes after this and see if they can be merged
             // into box i
             for (k = i + 1; k < n; k++)
               {
                  // skip empty boxes after i
                  if (box[k].x1 == box[k].x2) continue;
                  // match x coords
                  if ((box[i].x1 == box[k].x1) && // if aligned vertically
                      (box[i].x2 == box[k].x2))   // exactly above/below
                    {
                       // right below, or right above
                       if (box[i].y2 == box[k].y1) // this box flush below
                         {
                            box[i].y2 = box[k].y2; // merge below i
                            box[k].x2 = box[k].x1; // empty this box - merged
                         }
                       else if (box[i].y1 == box[k].y2) // this box flush above
                         {
                            box[i].y2 = box[k].y2; // merge above i
                            box[k].x2 = box[k].x1; // empty this box - merged
                         }
                    }
               }
             // i may have expanded but will not be empty. future boxes after
             // this may be empty though but handled at top of loop
             merged[j].x = box[i].x1;
             merged[j].y = box[i].y1;
             merged[j].w = box[i].x2 - box[i].x1;
             merged[j].h = box[i].y2 - box[i].y1;
             mergelist = (Tilebuf_Rect *)eina_inlist_append
                (EINA_INLIST_GET(mergelist), EINA_INLIST_GET(&(merged[j])));
             j++;
          }
#endif
   }
   region_free(region);
   rects = mergelist;

   return rects;
}

/**
 * @brief Merges multiple lists of redraw rectangles based on the swap mode and merge mode.
 * Combines rectangles from previous frames (r1-r4) with the current frame's
 * redraws stored in the tilebuffer `tb`. Applies either bounding box merging
 * or smart merging based on `merge_mode`.
 * @param merge_mode The merging strategy (MERGE_BOUNDING, MERGE_SMART).
 * @param tb The Tilebuf containing the current frame's redraws.
 * @param r1 Redraw rectangles from frame N.
 * @param r2 Redraw rectangles from frame N-1.
 * @param r3 Redraw rectangles from frame N-2.
 * @param r4 Redraw rectangles from frame N-3.
 * @return A new list of merged redraw rectangles for the current update cycle.
 */
static Tilebuf_Rect *
_merge_rects(Render_Output_Merge_Mode merge_mode,
             Tilebuf *tb,
             Tilebuf_Rect *r1,
             Tilebuf_Rect *r2,
             Tilebuf_Rect *r3,
             Tilebuf_Rect *r4)
{
   Tilebuf_Rect *r, *rects;

   if (r1)
     {
        EINA_INLIST_FOREACH(EINA_INLIST_GET(r1), r)
          {
             evas_common_tilebuf_add_redraw(tb, r->x, r->y, r->w, r->h);
          }
     }
   if (r2)
     {
        EINA_INLIST_FOREACH(EINA_INLIST_GET(r2), r)
          {
             evas_common_tilebuf_add_redraw(tb, r->x, r->y, r->w, r->h);
          }
     }
   if (r3)
     {
        EINA_INLIST_FOREACH(EINA_INLIST_GET(r3), r)
          {
             evas_common_tilebuf_add_redraw(tb, r->x, r->y, r->w, r->h);
          }
     }

   if (r4)
     {
        EINA_INLIST_FOREACH(EINA_INLIST_GET(r4), r)
          {
             evas_common_tilebuf_add_redraw(tb, r->x, r->y, r->w, r->h);
          }
     }

   rects = evas_common_tilebuf_get_render_rects(tb);
   // bounding box -> make a bounding box single region update of all regions.
   // yes we could try and be smart and figure out size of regions, how far
   // apart etc. etc. to try and figure out an optimal "set". this is a tradeoff
   // between multiple update regions to render and total pixels to render.
   if (rects)
     {
        if (merge_mode == MERGE_BOUNDING
// disable smart updates for debugging
//            || (merge_mode == MERGE_SMART)
            )
          {
             int px1, py1, px2, py2;

             px1 = rects->x; py1 = rects->y;
             px2 = rects->x + rects->w; py2 = rects->y + rects->h;
             EINA_INLIST_FOREACH(EINA_INLIST_GET(rects), r)
               {
                  if (r->x < px1) px1 = r->x;
                  if (r->y < py1) py1 = r->y;
                  if ((r->x + r->w) > px2) px2 = r->x + r->w;
                  if ((r->y + r->h) > py2) py2 = r->y + r->h;
               }
             evas_common_tilebuf_free_render_rects(rects);
             rects = calloc(1, sizeof(Tilebuf_Rect));
             if (rects)
               {
                  rects->x = px1;
                  rects->y = py1;
                  rects->w = px2 - px1;
                  rects->h = py2 - py1;
               }
          }
        else if (merge_mode == MERGE_SMART)
          {
             rects = _smart_merge(tb, rects);
          }
     }
   evas_common_tilebuf_clear(tb);
   return rects;
}


/**
 * @brief Gets the next rectangular region that needs to be redrawn and the surface to draw on.
 * This function manages the update cycle based on the output buffer's swap mode.
 * It retrieves redraws from the tilebuffer, merges them with previous frame redraws
 * according to the swap mode (COPY, DOUBLE, TRIPLE, etc.) and merge mode (BOUNDING, SMART),
 * and returns one update rectangle at a time along with a surface (obtained from the output buffer)
 * to render into.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 * @param x Pointer to store the logical x-coordinate of the update rectangle.
 * @param y Pointer to store the logical y-coordinate of the update rectangle.
 * @param w Pointer to store the logical width of the update rectangle.
 * @param h Pointer to store the logical height of the update rectangle.
 * @param cx Pointer to store the actual x-coordinate within the returned surface buffer.
 * @param cy Pointer to store the actual y-coordinate within the returned surface buffer.
 * @param cw Pointer to store the actual width within the returned surface buffer.
 * @param ch Pointer to store the actual height within the returned surface buffer.
 * @return A pointer to the surface buffer to draw into for this update region, or NULL if there are no more updates.
 */
static void *
eng_output_redraws_next_update_get(void *engine EINA_UNUSED, void *data, int *x, int *y, int *w, int *h, int *cx, int *cy, int *cw, int *ch)
{
   Render_Output_Software_Generic *re;
   void *surface;
   Tilebuf_Rect *rect;

#define CLEAR_PREV_RECTS(x) \
   do { \
      if (re->rects_prev[x]) \
        evas_common_tilebuf_free_render_rects(re->rects_prev[x]); \
      re->rects_prev[x] = NULL; \
   } while (0)

   re = (Render_Output_Software_Generic *)data;
   if (re->end)
     {
        re->end = 0;
        return NULL;
     }

   if (!re->rects)
     {
        int mode = MODE_COPY;

        re->rects = evas_common_tilebuf_get_render_rects(re->tb);
        if (re->rects)
          {
             // do anything needed for the first rect and update lost backbuffer if needed
             if (re->outbuf_region_first_rect)
               re->lost_back |= re->outbuf_region_first_rect(re->ob);

             if (re->outbuf_swap_mode_get) mode = re->outbuf_swap_mode_get(re->ob);
             re->swap_mode = mode;
             if ((re->lost_back) || (re->swap_mode == MODE_FULL) || (re->swap_mode == MODE_AUTO))
               {
                  /* if we lost our backbuffer since the last frame redraw all */
                  re->lost_back = 0;
                  evas_common_tilebuf_add_redraw(re->tb, 0, 0, re->w, re->h);
                  evas_common_tilebuf_free_render_rects(re->rects);
                  re->rects = evas_common_tilebuf_get_render_rects(re->tb);
               }
             /* ensure we get rid of previous rect lists we dont need if mode
              * changed/is appropriate */
             evas_common_tilebuf_clear(re->tb);
             CLEAR_PREV_RECTS(3);
             re->rects_prev[3] = re->rects_prev[2];
             re->rects_prev[2] = re->rects_prev[1];
             re->rects_prev[1] = re->rects_prev[0];
             re->rects_prev[0] = re->rects;
             re->rects = NULL;
             switch (re->swap_mode)
               {
                case MODE_AUTO:
                case MODE_FULL:
                case MODE_COPY: // no prev rects needed
                  re->rects = _merge_rects(re->merge_mode, re->tb, re->rects_prev[0], NULL, NULL, NULL);
                  break;
                case MODE_DOUBLE: // double mode - only 1 level of prev rect
                  re->rects = _merge_rects(re->merge_mode, re->tb, re->rects_prev[0], re->rects_prev[1], NULL, NULL);
                  break;
                case MODE_TRIPLE: // triple mode - 2 levels of prev rect
                  re->rects = _merge_rects(re->merge_mode, re->tb, re->rects_prev[0], re->rects_prev[1], re->rects_prev[2], NULL);
                  break;
                case MODE_QUADRUPLE: // keep all
                  re->rects = _merge_rects(re->merge_mode, re->tb, re->rects_prev[0], re->rects_prev[1], re->rects_prev[2], re->rects_prev[3]);
                  break;
                default:
                  break;
               }
          }
        evas_common_tilebuf_clear(re->tb);
        re->cur_rect = EINA_INLIST_GET(re->rects);
        if (re->cur_rect && re->outbuf_damage_region_set)
          re->outbuf_damage_region_set(re->ob, re->rects);
     }
   if (!re->cur_rect) return NULL;
   rect = (Tilebuf_Rect *)re->cur_rect;
   if (re->rects)
     {
        switch (re->swap_mode)
          {
           case MODE_COPY:
           case MODE_DOUBLE:
           case MODE_TRIPLE:
           case MODE_QUADRUPLE:
             rect = (Tilebuf_Rect *)re->cur_rect;
             *x = rect->x;
             *y = rect->y;
             *w = rect->w;
             *h = rect->h;
             *cx = rect->x;
             *cy = rect->y;
             *cw = rect->w;
             *ch = rect->h;
             re->cur_rect = re->cur_rect->next;
             break;
           case MODE_AUTO:
           case MODE_FULL:
             re->cur_rect = NULL;
             *x = 0;
             *y = 0;
             *w = re->w;
             *h = re->h;
             if (cx) *cx = 0;
             if (cy) *cy = 0;
             if (cw) *cw = re->w;
             if (ch) *ch = re->h;
             break;
           default:
             break;
          }
        surface = re->outbuf_new_region_for_update(re->ob,
                                                   *x, *y, *w, *h,
                                                   cx, cy, cw, ch);
        if ((re->swap_mode == MODE_AUTO) ||
            (re->swap_mode == MODE_FULL) ||
            (!surface))
          {
             evas_common_tilebuf_free_render_rects(re->rects);
             re->rects = NULL;
             re->end = 1;
          }
        return surface;
     }
   return NULL;
}

/**
 * @brief Pushes a completed update region back to the output buffer.
 * Called after rendering into the surface obtained from eng_output_redraws_next_update_get.
 * Signals the output buffer to display/copy the updated region. Frees the update surface if necessary.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 * @param surface The surface buffer that was rendered into.
 * @param x The logical x-coordinate of the updated rectangle.
 * @param y The logical y-coordinate of the updated rectangle.
 * @param w The logical width of the updated rectangle.
 * @param h The logical height of the updated rectangle.
 * @param render_mode Current render mode (used to skip async init).
 */
static void
eng_output_redraws_next_update_push(void *engine EINA_UNUSED, void *data, void *surface, int x, int y, int w, int h, Evas_Render_Mode render_mode)
{
   Render_Output_Software_Generic *re;

   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   re = (Render_Output_Software_Generic *)data;
#if defined(BUILD_PIPE_RENDER)
   evas_common_pipe_map_begin(surface);
#endif /* BUILD_PIPE_RENDER */
   re->outbuf_push_updated_region(re->ob, surface, x, y, w, h);
   if (re->outbuf_free_region_for_update)
     re->outbuf_free_region_for_update(re->ob, surface);
   evas_common_cpu_end_opt();
}

/**
 * @brief Flushes all pending updates for an output.
 * Called after all update regions for a frame have been pushed. Signals the output buffer
 * that the frame is complete. Frees the merged rectangle list.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 * @param render_mode Current render mode (used to skip async init).
 */
static void
eng_output_flush(void *engine EINA_UNUSED, void *data, Evas_Render_Mode render_mode)
{
   Render_Output_Software_Generic *re;

   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   re = (Render_Output_Software_Generic *)data;
   if (re->outbuf_flush) re->outbuf_flush(re->ob, re->rects_prev[0], re->rects, render_mode);
   if (re->rects && render_mode != EVAS_RENDER_MODE_ASYNC_INIT)
     {
        evas_common_tilebuf_free_render_rects(re->rects);
        re->rects = NULL;
     }
}

/**
 * @brief Performs idle-time flushing or cleanup for an output.
 * Calls the output buffer's idle flush function if available.
 * @param engine The engine instance (unused).
 * @param data Pointer to the Render_Output_Software_Generic structure for the output.
 */
static void
eng_output_idle_flush(void *engine EINA_UNUSED, void *data)
{
   Render_Output_Software_Generic *re;

   re = (Render_Output_Software_Generic *)data;
   if (re->outbuf_idle_flush) re->outbuf_idle_flush(re->ob);
}

// Ector functions

/**
 * @brief Creates a new Ector surface suitable for the software engine.
 * Instantiates an ECTOR_SOFTWARE_SURFACE.
 * @param engine The engine instance (unused).
 * @return A new Ector_Surface object (refcounted), or NULL on failure.
 */
static Ector_Surface *
eng_ector_create(void *engine EINA_UNUSED)
{
   Ector_Surface *ector;

   efl_domain_current_push(EFL_ID_DOMAIN_SHARED);
   ector = efl_add_ref(ECTOR_SOFTWARE_SURFACE_CLASS, NULL);
   efl_domain_current_pop();
   return ector;
}

/**
 * @brief Creates an Evas image surface to be used as a backing store for Ector rendering.
 * This is typically used for caching intermediate Ector results.
 * @param engine The engine instance.
 * @param width Width of the surface.
 * @param height Height of the surface.
 * @param error Pointer to store an error flag (EINA_TRUE on failure).
 * @return A pointer to the new RGBA_Image surface, or NULL on failure.
 */
static void*
eng_ector_surface_create(void *engine, int width, int height, int *error)
{
   void *surface;

   *error = EINA_FALSE;

   surface = eng_image_new_from_copied_data(engine, width, height, NULL, EINA_TRUE, EVAS_COLORSPACE_ARGB8888);
   if (!surface) *error = EINA_TRUE;

   return surface;
}

/**
 * @brief Destroys an Evas image surface previously created by eng_ector_surface_create.
 * @param engine The engine instance.
 * @param surface Pointer to the RGBA_Image surface to destroy.
 */
static void
eng_ector_surface_destroy(void *engine, void *surface)
{
   if (!surface) return;
   eng_image_free(engine, surface);
}

/**
 * @brief Stores an Ector backing surface in the engine's generic cache.
 * @param engine Pointer to the Render_Engine_Software_Generic instance.
 * @param key The cache key (typically the Ector_Surface pointer).
 * @param surface The RGBA_Image surface to store.
 */
static void
eng_ector_surface_cache_set(void *engine, void *key , void *surface)
{
   Render_Engine_Software_Generic *e = engine;

   generic_cache_data_set(e->surface_cache, key, surface);

}

/**
 * @brief Retrieves an Ector backing surface from the engine's generic cache.
 * @param engine Pointer to the Render_Engine_Software_Generic instance.
 * @param key The cache key.
 * @return The cached RGBA_Image surface, or NULL if not found.
 */
static void *
eng_ector_surface_cache_get(void *engine, void *key)
{
   Render_Engine_Software_Generic *e = engine;

   return generic_cache_data_get(e->surface_cache, key);
}

/**
 * @brief Removes an Ector backing surface from the engine's generic cache (and potentially frees it).
 * @param engine Pointer to the Render_Engine_Software_Generic instance.
 * @param key The cache key.
 */
static void
eng_ector_surface_cache_drop(void *engine, void *key)
{
   Render_Engine_Software_Generic *e = engine;

   generic_cache_data_drop(e->surface_cache, key);
}

/**
 * @brief Destroys an Ector surface object.
 * Decrements the reference count of the Ector_Surface.
 * @param data Engine-specific data (unused).
 * @param ector The Ector_Surface to destroy/unref.
 */
static void
eng_ector_destroy(void *data EINA_UNUSED, Ector_Surface *ector)
{
   if (ector) efl_unref(ector);
}

/**
 * @brief Wraps an existing Evas image (Image_Entry) into an Ector_Buffer.
 * Creates an EVAS_ECTOR_SOFTWARE_BUFFER that references the Evas image data.
 * @param data Engine-specific data (passed to buffer).
 * @param e The Evas canvas (unused).
 * @param engine_image Pointer to the Evas Image_Entry to wrap.
 * @return A new Ector_Buffer object (refcounted) wrapping the image, or NULL on failure.
 */
static Ector_Buffer *
eng_ector_buffer_wrap(void *data, Evas *e EINA_UNUSED, void *engine_image)
{
   Image_Entry *ie = engine_image;
   Ector_Buffer *buf = NULL;
   RGBA_Image *im = (RGBA_Image *)ie;

   if (!ie) return NULL;
   if (!im->image.data) return NULL;

   if (!efl_domain_current_push(EFL_ID_DOMAIN_SHARED))
     return NULL;
   buf = efl_add_ref(EVAS_ECTOR_SOFTWARE_BUFFER_CLASS, NULL,
                 evas_ector_buffer_engine_image_set(efl_added, data, ie));
   efl_domain_current_pop();

   return buf;
}

/**
 * @brief Creates a new Ector_Buffer backed by a newly allocated Evas image.
 * Allocates an Evas image with the specified dimensions and colorspace, then wraps it.
 * @param data Engine-specific data (passed to buffer).
 * @param evas The Evas canvas.
 * @param width Width of the buffer.
 * @param height Height of the buffer.
 * @param cspace Colorspace of the buffer (currently supports ARGB8888, GRY8).
 * @param flags Buffer flags (unused).
 * @return A new Ector_Buffer object (refcounted), or NULL on failure.
 */
static Ector_Buffer *
eng_ector_buffer_new(void *data, Evas *evas, int width, int height,
                     Efl_Gfx_Colorspace cspace, Ector_Buffer_Flag flags EINA_UNUSED)
{
   Ector_Buffer *buf;
   Image_Entry *ie;
   void *pixels;
   int pxs;

   if (cspace == EFL_GFX_COLORSPACE_ARGB8888)
     pxs = 4;
   else if (cspace == EFL_GFX_COLORSPACE_GRY8)
     pxs = 1;
   else
     {
        ERR("Unsupported colorspace: %d", (int) cspace);
        return NULL;
     }

   // alloc buffer
   ie = evas_cache_image_copied_data(evas_common_image_cache_get(),
                                     width, height, NULL, EINA_TRUE,
                                     (Evas_Colorspace)cspace);
   if (!ie) return NULL;
   pixels = ((RGBA_Image *) ie)->image.data;
   memset(pixels, 0, width * height * pxs);

   buf = eng_ector_buffer_wrap(data, evas, ie);
   evas_cache_image_drop(ie);

   return buf;
}

/**
 * @brief Cleans up resources associated with a threaded Ector draw command.
 * Frees the duplicated clip rectangles and optionally the command structure itself.
 * @param ector The Evas_Thread_Command_Ector structure to clean up.
 */
static void
_draw_thread_ector_cleanup(Evas_Thread_Command_Ector *ector)
{
   Eina_Rectangle *r;

   while ((r = eina_array_pop(ector->clips)))
     eina_rectangle_free(r);
   eina_array_free(ector->clips);

   if (ector->free_it)
     eina_mempool_free(_mp_command_ector, ector);
}

/**
 * @brief Executes an Ector renderer draw command in a rendering thread.
 * Calls ector_renderer_draw with the provided parameters.
 * This function is called by the thread pool. Cleans up resources afterwards.
 * @param data Pointer to an Evas_Thread_Command_Ector structure.
 */
static void
_draw_thread_ector_draw(void *data)
{
   Evas_Thread_Command_Ector *ector = data;

   ector_renderer_draw(ector->r, ector->render_op, ector->clips, ector->mul_col);

   _draw_thread_ector_cleanup(ector);
}

/**
 * @brief Engine function to execute an Ector renderer's drawing operations onto an Evas surface.
 * Clips the provided clip rectangles against the Evas drawing context's clip region.
 * Dispatches the drawing to the appropriate implementation (sync or async thread).
 * @param engine The engine instance (unused).
 * @param surface The target Evas surface (RGBA_Image).
 * @param context The Evas drawing context (used for clipping).
 * @param renderer The Ector_Renderer containing the drawing commands.
 * @param clips An Eina_Array of Eina_Rectangle* defining clipping areas for the Ector draw.
 * @param do_async If true, attempt asynchronous (threaded) drawing.
 */
static void
eng_ector_renderer_draw(void *engine EINA_UNUSED, void *surface,
                        void *context, Ector_Renderer *renderer,
                        Eina_Array *clips, Eina_Bool do_async)
{
   RGBA_Image *dst = surface;
   RGBA_Draw_Context *dc = context;
   Evas_Thread_Command_Ector ector;
   Eina_Array *c;
   Eina_Rectangle *r;
   Eina_Rectangle clip;
   Eina_Array_Iterator it;
   unsigned int i;

   if (dc->clip.use)
     {
        clip.x = dc->clip.x;
        clip.y = dc->clip.y;
        clip.w = dc->clip.w;
        clip.h = dc->clip.h;
        // clip the clip rect to surface boundary.
        RECTS_CLIP_TO_RECT(clip.x, clip.y, clip.w, clip.h, 0, 0, dst->cache_entry.w, dst->cache_entry.h);
        if ((clip.w < 1) || (clip.h < 1)) return;
     }
   else
     {
        clip.x = 0;
        clip.y = 0;
        clip.w = dst->cache_entry.w;
        clip.h = dst->cache_entry.h;
     }

   c = eina_array_new(8);
   if (clips)
     {
        EINA_ARRAY_ITER_NEXT(clips, i, r, it)
          {
             Eina_Rectangle *rc;

             rc = eina_rectangle_new(r->x, r->y, r->w, r->h);
             if (!rc) continue;

             if (eina_rectangle_intersection(rc, &clip))
               eina_array_push(c, rc);
             else
               eina_rectangle_free(rc);
          }

        if (eina_array_count(c) == 0 &&
            eina_array_count(clips) > 0)
          {
             eina_array_free(c);
             return;
          }
     }

   if (eina_array_count(c) == 0)
     eina_array_push(c, eina_rectangle_new(clip.x, clip.y, clip.w, clip.h));

   ector.r = renderer; // This has already been refcounted by Evas_Object_VG
   ector.clips = c;
   ector.render_op = EFL_GFX_RENDER_OP_BLEND;
   ector.mul_col = 0xffffffff;
   ector.free_it = EINA_FALSE;

   if (do_async)
     {
        Evas_Thread_Command_Ector *ne;

        ne = eina_mempool_malloc(_mp_command_ector, sizeof (Evas_Thread_Command_Ector));
        if (!ne)
          {
             _draw_thread_ector_cleanup(&ector);
             return;
          }

        memcpy(ne, &ector, sizeof (Evas_Thread_Command_Ector));
        ne->free_it = EINA_TRUE;

        QCMD(_draw_thread_ector_draw, ne);
     }
   else
     {
        _draw_thread_ector_draw(&ector);
     }
}

/**
 * @brief Executes an Ector surface setup command in a rendering thread.
 * Sets the pixel buffer, dimensions, and reference point for an Ector_Surface.
 * If setting a buffer, clears it first. If unsetting (pixels=NULL), just updates Ector.
 * This function is called by the thread pool. Frees the command structure afterwards.
 * @param data Pointer to an Evas_Thread_Command_Ector_Surface structure.
 */
static void
_draw_thread_ector_surface_set(void *data)
{
   Evas_Thread_Command_Ector_Surface *ector_surface = data;
   RGBA_Image *surface = ector_surface->pixels;
   void *pixels = NULL;
   unsigned int w = 0;
   unsigned int h = 0;
   unsigned int x = 0;
   unsigned int y = 0;

   // flush the cpu pipeline before ector drawing.
   evas_common_cpu_end_opt();

   if (surface)
     {
        pixels = evas_cache_image_pixels(&surface->cache_entry);
        if (pixels)
          {
             w = surface->cache_entry.w;
             h = surface->cache_entry.h;
             x = ector_surface->x;
             y = ector_surface->y;
             // clear the surface before giving to ector
             memset(pixels, 0, (w * h * 4));
          }
     }

   ector_buffer_pixels_set(ector_surface->ector, pixels, w, h, 0, EFL_GFX_COLORSPACE_ARGB8888, EINA_TRUE);
   ector_surface_reference_point_set(ector_surface->ector, x, y);

   eina_mempool_free(_mp_command_ector_surface, ector_surface);
}

/**
 * @brief Prepares an Ector_Surface to render onto an Evas surface (RGBA_Image).
 * Associates the Evas surface's pixel buffer with the Ector_Surface, sets the
 * reference point, and clears the buffer. Dispatches to sync or async thread.
 * @param engine The engine instance (unused).
 * @param surface The target Evas surface (RGBA_Image).
 * @param context The Evas drawing context (unused).
 * @param ector The Ector_Surface to prepare.
 * @param x The x-coordinate reference point for the Ector surface.
 * @param y The y-coordinate reference point for the Ector surface.
 * @param do_async If true, perform setup asynchronously in a thread.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., couldn't get pixels).
 */
static Eina_Bool
eng_ector_begin(void *engine EINA_UNUSED, void *surface,
                void *context EINA_UNUSED, Ector_Surface *ector,
                int x, int y, Eina_Bool do_async)
{
   if (do_async)
     {
        Evas_Thread_Command_Ector_Surface *nes;

        nes = eina_mempool_malloc(_mp_command_ector_surface, sizeof (Evas_Thread_Command_Ector_Surface));
        if (!nes) return EINA_FALSE;

        nes->ector = ector;
        nes->pixels = surface;
        nes->x = x;
        nes->y = y;

        QCMD(_draw_thread_ector_surface_set, nes);
     }
   else
     {
        RGBA_Image *sf = surface;
        void *pixels = NULL;
        unsigned int w = 0;
        unsigned int h = 0;

        pixels = evas_cache_image_pixels(&sf->cache_entry);
        if (!pixels) return EINA_FALSE;

        w = sf->cache_entry.w;
        h = sf->cache_entry.h;
        // clear the surface before giving to ector
        memset(pixels, 0, (w * h * 4));

        ector_buffer_pixels_set(ector, pixels, w, h, 0, EFL_GFX_COLORSPACE_ARGB8888, EINA_TRUE);
        ector_surface_reference_point_set(ector, x, y);
     }
   return EINA_TRUE;
}

/**
 * @brief Finalizes rendering to an Ector_Surface associated with an Evas surface.
 * Detaches the pixel buffer from the Ector_Surface by setting it to NULL.
 * Dispatches to sync or async thread.
 * @param engine The engine instance (unused).
 * @param surface The target Evas surface (unused).
 * @param context The Evas drawing context (unused).
 * @param ector The Ector_Surface to finalize.
 * @param do_async If true, perform finalization asynchronously in a thread.
 */
static void
eng_ector_end(void *engine EINA_UNUSED,
              void *surface EINA_UNUSED,
              void *context EINA_UNUSED,
              Ector_Surface *ector,
              Eina_Bool do_async)
{
   if (do_async)
     {
        Evas_Thread_Command_Ector_Surface *nes;

        nes = eina_mempool_malloc(_mp_command_ector_surface, sizeof (Evas_Thread_Command_Ector_Surface));
        if (!nes) return ;

        nes->ector = ector;
        nes->pixels = NULL;

        QCMD(_draw_thread_ector_surface_set, nes);
     }
   else
     {
        ector_buffer_pixels_set(ector, NULL, 0, 0, 0, EFL_GFX_COLORSPACE_ARGB8888, EINA_TRUE);
        evas_common_cpu_end_opt();
     }
}

//------------------------------------------------//

/**
 * @brief Gets the appropriate software filter function based on the filter mode.
 * @param cmd The Evas_Filter_Command describing the filter operation.
 * @return A function pointer (Software_Filter_Func) to the implementation, or NULL if unsupported.
 */
static Software_Filter_Func
_gfx_filter_func_get(Evas_Filter_Command *cmd)
{
   Software_Filter_Func func = NULL;

   switch (cmd->mode)
     {
      case EVAS_FILTER_MODE_BLEND: func = eng_filter_blend_func_get(cmd); break;
      case EVAS_FILTER_MODE_BLUR: func = eng_filter_blur_func_get(cmd); break;
      case EVAS_FILTER_MODE_BUMP: func = eng_filter_bump_func_get(cmd); break;
      case EVAS_FILTER_MODE_CURVE: func = eng_filter_curve_func_get(cmd); break;
      case EVAS_FILTER_MODE_DISPLACE: func = eng_filter_displace_func_get(cmd); break;
      case EVAS_FILTER_MODE_FILL: func = eng_filter_fill_func_get(cmd); break;
      case EVAS_FILTER_MODE_MASK: func = eng_filter_mask_func_get(cmd); break;
      case EVAS_FILTER_MODE_TRANSFORM: func = eng_filter_transform_func_get(cmd); break;
      case EVAS_FILTER_MODE_GRAYSCALE: func = eng_filter_grayscale_func_get(cmd); break;
      case EVAS_FILTER_MODE_INVERSE_COLOR: func = eng_filter_inverse_color_func_get(cmd); break;
      default: return NULL;
     }

   return func;
}

/**
 * @brief Checks if a graphics filter command is supported by the software engine.
 * @param data Engine-specific data (unused).
 * @param cmd The Evas_Filter_Command to check.
 * @return EVAS_FILTER_SUPPORT_CPU if supported, EVAS_FILTER_SUPPORT_NONE otherwise.
 */
static Evas_Filter_Support
eng_gfx_filter_supports(void *data EINA_UNUSED, Evas_Filter_Command *cmd)
{
   if (!_gfx_filter_func_get(cmd))
     return EVAS_FILTER_SUPPORT_NONE;

   return EVAS_FILTER_SUPPORT_CPU;
}

/**
 * @brief Triggers garbage collection for font glyphs/atlases.
 * This is a no-op in the software engine as it doesn't use texture atlases for fonts.
 * @param data Engine-specific data (unused).
 * @param ratio Ratio of glyphs to collect (unused).
 * @param texture_size Pointer to store texture size (unused).
 * @param atlas_size Pointer to store atlas size (unused).
 * @param only_when_requested Flag (unused).
 */
static void
eng_font_glyphs_gc_collect(void *data EINA_UNUSED, float ratio EINA_UNUSED, int *texture_size EINA_UNUSED, int *atlas_size EINA_UNUSED, Eina_Bool only_when_requested EINA_UNUSED)
{
   return;
}

/**
 * @brief Processes a graphics filter command using the software implementation.
 * Finds the appropriate filter function and executes it.
 * @param data Engine-specific data (unused).
 * @param cmd The Evas_Filter_Command to process.
 * @return EINA_TRUE on success, EINA_FALSE if the filter is unsupported or fails.
 */
static Eina_Bool
eng_gfx_filter_process(void *data EINA_UNUSED, Evas_Filter_Command *cmd)
{
   Software_Filter_Func func;

   func = _gfx_filter_func_get(cmd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, EINA_FALSE);

   return func(cmd);
}

//------------------------------------------------//

/*
 *****
 **
 ** ENGINE API
 **
 *****
 */

static Evas_Func func =
{
     eng_engine_new,
     eng_engine_free,
     NULL, // eng_info_setup
     NULL, // eng_setup
     NULL, // eng_update
     NULL, // eng_output_free
     eng_output_resize,
     eng_output_redraws_rect_add,
     eng_output_redraws_rect_del,
     eng_output_redraws_clear,
     eng_output_redraws_next_update_get,
     eng_output_redraws_next_update_push,
     eng_output_flush,
     eng_output_idle_flush,
     eng_output_dump,
     /* draw context virtual methods */
     eng_context_new,
     eng_context_dup,
     eng_canvas_alpha_get,
     eng_context_free,
     eng_context_clip_set,
     eng_context_clip_image_set,
     eng_context_clip_image_unset,
     eng_context_clip_image_get,
     eng_context_clip_clip,
     eng_context_clip_unset,
     eng_context_clip_get,
     eng_context_color_set,
     eng_context_color_get,
     eng_context_multiplier_set,
     eng_context_multiplier_unset,
     eng_context_multiplier_get,
     eng_context_cutout_add,
     eng_context_cutout_clear,
     eng_context_cutout_target,
     eng_context_anti_alias_set,
     eng_context_anti_alias_get,
     eng_context_color_interpolation_set,
     eng_context_color_interpolation_get,
     eng_context_render_op_set,
     eng_context_render_op_get,
     /* rect draw funcs */
     eng_rectangle_draw,
     /* line draw funcs */
     eng_line_draw,
     /* polygon draw funcs */
     eng_polygon_point_add,
     eng_polygon_points_clear,
     eng_polygon_draw,
     /* image draw funcs */
     eng_image_load,
     eng_image_mmap,
     eng_image_new_from_data,
     eng_image_new_from_copied_data,
     eng_image_free,
     eng_image_ref,
     eng_image_size_get,
     eng_image_size_set,
     NULL, // eng_image_stride_get
     eng_image_dirty_region,
     eng_image_data_get,
     eng_image_data_put,
     eng_image_data_direct_get,
     eng_image_data_preload_request,
     eng_image_data_preload_cancel,
     eng_image_alpha_set,
     eng_image_alpha_get,
     eng_image_orient_set,
     eng_image_orient_get,
     eng_image_draw,
     eng_image_colorspace_set,
     eng_image_colorspace_get,
     eng_image_file_colorspace_get,
     eng_image_can_region_get,
     eng_image_data_map,
     eng_image_data_unmap,
     eng_image_data_maps_get,
     eng_image_content_region_get,
     eng_image_stretch_region_get,
     eng_image_data_slice_add,
     eng_image_prepare,
     eng_image_surface_noscale_new,
     eng_image_native_init,
     eng_image_native_shutdown,
     eng_image_native_set,
     eng_image_native_get,
     /* image cache funcs */
     eng_image_cache_flush,
     eng_image_cache_set,
     eng_image_cache_get,
     NULL, // image_plane_assign
     NULL, // image_plane_release
     /* font draw functions */
     eng_font_load,
     eng_font_memory_load,
     eng_font_add,
     eng_font_memory_add,
     eng_font_free,
     eng_font_ascent_get,
     eng_font_descent_get,
     eng_font_max_ascent_get,
     eng_font_max_descent_get,
     eng_font_string_size_get,
     eng_font_inset_get,
     eng_font_h_advance_get,
     eng_font_v_advance_get,
     eng_font_char_coords_get,
     eng_font_char_at_coords_get,
     eng_font_draw,
     /* font cache functions */
     eng_font_cache_flush,
     eng_font_cache_set,
     eng_font_cache_get,
     /* font hinting functions */
     eng_font_hinting_set,
     eng_font_hinting_can_hint,
     eng_image_scale_hint_set,
     eng_image_scale_hint_get,
     /* more font draw functions */
     eng_font_last_up_to_pos,
     eng_image_map_draw,
     eng_image_map_surface_new,
     eng_image_map_clean,
     NULL, // eng_image_scaled_get - used for live scaling in GL only (fastpath)
     NULL, // eng_image_content_hint_set - software doesn't use it
     eng_font_pen_coords_get,
     eng_font_text_props_info_create,
     eng_font_right_inset_get,
     eng_gl_supports_evas_gl, // returns true iif OSMesa is present
     NULL, // No need to set output for software engine
     eng_gl_surface_create, // need software mesa for gl rendering <- gl_surface_create
     NULL, // need software mesa for gl rendering <- gl_pbuffer_surface_create
     eng_gl_surface_destroy, // need software mesa for gl rendering <- gl_surface_destroy
     eng_gl_context_create, // need software mesa for gl rendering <- gl_context_create
     eng_gl_context_destroy, // need software mesa for gl rendering <- gl_context_destroy
     eng_gl_make_current, // need software mesa for gl rendering <- gl_make_current
     eng_gl_string_query, // need software mesa for gl rendering <- gl_string_query
     eng_gl_proc_address_get, // need software mesa for gl rendering <- gl_proc_address_get
     eng_gl_native_surface_get, // need software mesa for gl rendering <- gl_native_surface_get
       eng_gl_api_get, // need software mesa for gl rendering <- gl_api_get
     NULL, // need software mesa for gl rendering <- gl_direct_override
     NULL, // need software mesa for gl rendering <- gl_get_pixels_set
     NULL, // need software mesa for gl rendering <- gl_surface_lock
     NULL, // need software mesa for gl rendering <- gl_surface_read_pixels
     NULL, // need software mesa for gl rendering <- gl_surface_unlock
     eng_gl_error_get, // need software mesa for gl rendering <- gl_error_get
     eng_gl_current_context_get, // need software mesa for gl rendering <- gl_current_context_get
     eng_gl_current_surface_get, // need software mesa for gl rendering <- gl_current_surface_get
     eng_gl_rotation_angle_get, // need software mesa for gl rendering <- gl_rotation_angle_get
     NULL, // need software mesa for gl rendering <- gl_surface_query
     NULL, // need software mesa for gl rendering <- gl_surface_direct_renderable_get
     NULL, // need software mesa for gl rendering <- gl_image_direct_set
     NULL, // need software mesa for gl rendering <- gl_image_direct_get
     NULL, // need software mesa for gl rendering <- gl_get_pixels_pre
     NULL, // need software mesa for gl rendering <- gl_get_pixels_post
     eng_image_load_error_get,
     eng_font_run_font_end_get,
     eng_image_animated_get,
     eng_image_animated_frame_count_get,
     eng_image_animated_loop_type_get,
     eng_image_animated_loop_count_get,
     eng_image_animated_frame_duration_get,
     eng_image_animated_frame_set,
     eng_image_animated_frame_get,
     NULL, // image_max_size_get
     eng_multi_font_draw,
     eng_pixel_alpha_get,
     NULL, // eng_context_flush - software doesn't use it
     eng_ector_create,
     eng_ector_destroy,
     eng_ector_buffer_wrap,
     eng_ector_buffer_new,
     eng_ector_begin,
     eng_ector_renderer_draw,
     eng_ector_end,
     eng_ector_surface_create,
     eng_ector_surface_destroy,
     eng_ector_surface_cache_set,
     eng_ector_surface_cache_get,
     eng_ector_surface_cache_drop,
     eng_gfx_filter_supports,
     eng_gfx_filter_process,
   /* FUTURE software generic calls go here */
     eng_font_glyphs_gc_collect,
     0 // sizeof (Info)
};


//----------------------------------------------------------------//
//                                                                //
//                      Load Symbols                              //
//                                                                //
//----------------------------------------------------------------//
#ifdef EVAS_GL
/**
 * @brief Placeholder function used when a required GL symbol cannot be found. Logs an error.
 */
static void
sym_missing(void)
{
   ERR("GL symbols missing!");
}

/**
 * @brief Initializes core OSMesa API function pointers required by the engine.
 * Uses dlsym to find the symbols in the loaded OSMesa library.
 * @return 1 on success (all required symbols found), 0 on failure.
 */
static int
glue_sym_init(void)
{
   //------------------------------------------------//
   // Use eglGetProcAddress
#define FINDSYM(dst, sym, typ) \
   if (!dst) dst = dlsym(gl_lib_handle, sym); \
   if (!dst)  \
     { \
        ERR("Symbol not found: %s", sym); \
        return 0; \
     }
#define FALLBAK(dst, typ) if (!dst) dst = (typeof(dst))sym_missing;

    //------------------------------------------------------//
   // OSMesa APIs...
   FINDSYM(_sym_OSMesaCreateContextExt, "OSMesaCreateContextExt", glsym_func_osm_ctx);
   FALLBAK(_sym_OSMesaCreateContextExt, glsym_func_void);

   FINDSYM(_sym_OSMesaDestroyContext, "OSMesaDestroyContext", glsym_func_void);
   FALLBAK(_sym_OSMesaDestroyContext, glsym_func_void);

   FINDSYM(_sym_OSMesaMakeCurrent, "OSMesaMakeCurrent", glsym_func_bool);
   FALLBAK(_sym_OSMesaMakeCurrent, glsym_func_void);

   FINDSYM(_sym_OSMesaPixelStore, "OSMesaPixelStore", glsym_func_void);
   FALLBAK(_sym_OSMesaPixelStore, glsym_func_void);

   FINDSYM(_sym_OSMesaGetProcAddress, "OSMesaGetProcAddress", glsym_func_eng_fn);
   FALLBAK(_sym_OSMesaGetProcAddress, glsym_func_void);

#undef FINDSYM
#undef FALLBAK

   return 1;
}

/**
 * @brief Initializes function pointers for OpenGL (ES 2.0) APIs.
 * Attempts to find symbols using dlsym and falls back to OSMesaGetProcAddress.
 * Assigns sym_missing as a fallback if a symbol is not found.
 * Determines if the underlying library is GLES or desktop GL based on symbol availability.
 * @return EINA_TRUE if all essential symbols were found, EINA_FALSE otherwise.
 */
static Eina_Bool
gl_sym_init(void)
{
   Eina_Bool ok = EINA_TRUE;

   //------------------------------------------------//
#define FINDSYM(dst, sym, typ) do { \
   if (!dst) dst = dlsym(gl_lib_handle, sym); \
   if (!dst && _sym_OSMesaGetProcAddress) dst = (void *)_sym_OSMesaGetProcAddress(sym); \
   if (!dst) DBG("Symbol not found: %s", sym); \
   } while (0)
#define FALLBAK(dst, typ) do { \
   if (!dst) { dst = (void *)sym_missing; ok = EINA_FALSE; } \
   } while (0)

   //------------------------------------------------------//
   // GLES 2.0 APIs...
   FINDSYM(_sym_glActiveTexture, "glActiveTexture", glsym_func_void);
   FALLBAK(_sym_glActiveTexture, glsym_func_void);

   FINDSYM(_sym_glAttachShader, "glAttachShader", glsym_func_void);
   FALLBAK(_sym_glAttachShader, glsym_func_void);

   FINDSYM(_sym_glBindAttribLocation, "glBindAttribLocation", glsym_func_void);
   FALLBAK(_sym_glBindAttribLocation, glsym_func_void);

   FINDSYM(_sym_glBindBuffer, "glBindBuffer", glsym_func_void);
   FALLBAK(_sym_glBindBuffer, glsym_func_void);

   FINDSYM(_sym_glBindFramebuffer, "glBindFramebuffer", glsym_func_void);
   FALLBAK(_sym_glBindFramebuffer, glsym_func_void);

   FINDSYM(_sym_glBindRenderbuffer, "glBindRenderbuffer", glsym_func_void);
   FALLBAK(_sym_glBindRenderbuffer, glsym_func_void);

   FINDSYM(_sym_glBindTexture, "glBindTexture", glsym_func_void);
   FALLBAK(_sym_glBindTexture, glsym_func_void);

   FINDSYM(_sym_glBlendColor, "glBlendColor", glsym_func_void);
   FALLBAK(_sym_glBlendColor, glsym_func_void);

   FINDSYM(_sym_glBlendEquation, "glBlendEquation", glsym_func_void);
   FALLBAK(_sym_glBlendEquation, glsym_func_void);

   FINDSYM(_sym_glBlendEquationSeparate, "glBlendEquationSeparate", glsym_func_void);
   FALLBAK(_sym_glBlendEquationSeparate, glsym_func_void);

   FINDSYM(_sym_glBlendFunc, "glBlendFunc", glsym_func_void);
   FALLBAK(_sym_glBlendFunc, glsym_func_void);

   FINDSYM(_sym_glBlendFuncSeparate, "glBlendFuncSeparate", glsym_func_void);
   FALLBAK(_sym_glBlendFuncSeparate, glsym_func_void);

   FINDSYM(_sym_glBufferData, "glBufferData", glsym_func_void);
   FALLBAK(_sym_glBufferData, glsym_func_void);

   FINDSYM(_sym_glBufferSubData, "glBufferSubData", glsym_func_void);
   FALLBAK(_sym_glBufferSubData, glsym_func_void);

   FINDSYM(_sym_glCheckFramebufferStatus, "glCheckFramebufferStatus", glsym_func_uint);
   FALLBAK(_sym_glCheckFramebufferStatus, glsym_func_uint);

   FINDSYM(_sym_glClear, "glClear", glsym_func_void);
   FALLBAK(_sym_glClear, glsym_func_void);

   FINDSYM(_sym_glClearColor, "glClearColor", glsym_func_void);
   FALLBAK(_sym_glClearColor, glsym_func_void);

   FINDSYM(_sym_glClearDepthf, "glClearDepthf", glsym_func_void);
   FINDSYM(_sym_glClearDepthf, "glClearDepth", glsym_func_void);
   FALLBAK(_sym_glClearDepthf, glsym_func_void);

   FINDSYM(_sym_glClearStencil, "glClearStencil", glsym_func_void);
   FALLBAK(_sym_glClearStencil, glsym_func_void);

   FINDSYM(_sym_glColorMask, "glColorMask", glsym_func_void);
   FALLBAK(_sym_glColorMask, glsym_func_void);

   FINDSYM(_sym_glCompileShader, "glCompileShader", glsym_func_void);
   FALLBAK(_sym_glCompileShader, glsym_func_void);

   FINDSYM(_sym_glCompressedTexImage2D, "glCompressedTexImage2D", glsym_func_void);
   FALLBAK(_sym_glCompressedTexImage2D, glsym_func_void);

   FINDSYM(_sym_glCompressedTexSubImage2D, "glCompressedTexSubImage2D", glsym_func_void);
   FALLBAK(_sym_glCompressedTexSubImage2D, glsym_func_void);

   FINDSYM(_sym_glCopyTexImage2D, "glCopyTexImage2D", glsym_func_void);
   FALLBAK(_sym_glCopyTexImage2D, glsym_func_void);

   FINDSYM(_sym_glCopyTexSubImage2D, "glCopyTexSubImage2D", glsym_func_void);
   FALLBAK(_sym_glCopyTexSubImage2D, glsym_func_void);

   FINDSYM(_sym_glCreateProgram, "glCreateProgram", glsym_func_uint);
   FALLBAK(_sym_glCreateProgram, glsym_func_uint);

   FINDSYM(_sym_glCreateShader, "glCreateShader", glsym_func_uint);
   FALLBAK(_sym_glCreateShader, glsym_func_uint);

   FINDSYM(_sym_glCullFace, "glCullFace", glsym_func_void);
   FALLBAK(_sym_glCullFace, glsym_func_void);

   FINDSYM(_sym_glDeleteBuffers, "glDeleteBuffers", glsym_func_void);
   FALLBAK(_sym_glDeleteBuffers, glsym_func_void);

   FINDSYM(_sym_glDeleteFramebuffers, "glDeleteFramebuffers", glsym_func_void);
   FALLBAK(_sym_glDeleteFramebuffers, glsym_func_void);

   FINDSYM(_sym_glDeleteProgram, "glDeleteProgram", glsym_func_void);
   FALLBAK(_sym_glDeleteProgram, glsym_func_void);

   FINDSYM(_sym_glDeleteRenderbuffers, "glDeleteRenderbuffers", glsym_func_void);
   FALLBAK(_sym_glDeleteRenderbuffers, glsym_func_void);

   FINDSYM(_sym_glDeleteShader, "glDeleteShader", glsym_func_void);
   FALLBAK(_sym_glDeleteShader, glsym_func_void);

   FINDSYM(_sym_glDeleteTextures, "glDeleteTextures", glsym_func_void);
   FALLBAK(_sym_glDeleteTextures, glsym_func_void);

   FINDSYM(_sym_glDepthFunc, "glDepthFunc", glsym_func_void);
   FALLBAK(_sym_glDepthFunc, glsym_func_void);

   FINDSYM(_sym_glDepthMask, "glDepthMask", glsym_func_void);
   FALLBAK(_sym_glDepthMask, glsym_func_void);

   FINDSYM(_sym_glDepthRangef, "glDepthRangef", glsym_func_void);
   FINDSYM(_sym_glDepthRangef, "glDepthRange", glsym_func_void);
   FALLBAK(_sym_glDepthRangef, glsym_func_void);

   FINDSYM(_sym_glDetachShader, "glDetachShader", glsym_func_void);
   FALLBAK(_sym_glDetachShader, glsym_func_void);

   FINDSYM(_sym_glDisable, "glDisable", glsym_func_void);
   FALLBAK(_sym_glDisable, glsym_func_void);

   FINDSYM(_sym_glDisableVertexAttribArray, "glDisableVertexAttribArray", glsym_func_void);
   FALLBAK(_sym_glDisableVertexAttribArray, glsym_func_void);

   FINDSYM(_sym_glDrawArrays, "glDrawArrays", glsym_func_void);
   FALLBAK(_sym_glDrawArrays, glsym_func_void);

   FINDSYM(_sym_glDrawElements, "glDrawElements", glsym_func_void);
   FALLBAK(_sym_glDrawElements, glsym_func_void);

   FINDSYM(_sym_glEnable, "glEnable", glsym_func_void);
   FALLBAK(_sym_glEnable, glsym_func_void);

   FINDSYM(_sym_glEnableVertexAttribArray, "glEnableVertexAttribArray", glsym_func_void);
   FALLBAK(_sym_glEnableVertexAttribArray, glsym_func_void);

   FINDSYM(_sym_glFinish, "glFinish", glsym_func_void);
   FALLBAK(_sym_glFinish, glsym_func_void);

   FINDSYM(_sym_glFlush, "glFlush", glsym_func_void);
   FALLBAK(_sym_glFlush, glsym_func_void);

   FINDSYM(_sym_glFramebufferRenderbuffer, "glFramebufferRenderbuffer", glsym_func_void);
   FALLBAK(_sym_glFramebufferRenderbuffer, glsym_func_void);

   FINDSYM(_sym_glFramebufferTexture2D, "glFramebufferTexture2D", glsym_func_void);
   FALLBAK(_sym_glFramebufferTexture2D, glsym_func_void);

   FINDSYM(_sym_glFrontFace, "glFrontFace", glsym_func_void);
   FALLBAK(_sym_glFrontFace, glsym_func_void);

   FINDSYM(_sym_glGenBuffers, "glGenBuffers", glsym_func_void);
   FALLBAK(_sym_glGenBuffers, glsym_func_void);

   FINDSYM(_sym_glGenerateMipmap, "glGenerateMipmap", glsym_func_void);
   FALLBAK(_sym_glGenerateMipmap, glsym_func_void);

   FINDSYM(_sym_glGenFramebuffers, "glGenFramebuffers", glsym_func_void);
   FALLBAK(_sym_glGenFramebuffers, glsym_func_void);

   FINDSYM(_sym_glGenRenderbuffers, "glGenRenderbuffers", glsym_func_void);
   FALLBAK(_sym_glGenRenderbuffers, glsym_func_void);

   FINDSYM(_sym_glGenTextures, "glGenTextures", glsym_func_void);
   FALLBAK(_sym_glGenTextures, glsym_func_void);

   FINDSYM(_sym_glGetActiveAttrib, "glGetActiveAttrib", glsym_func_void);
   FALLBAK(_sym_glGetActiveAttrib, glsym_func_void);

   FINDSYM(_sym_glGetActiveUniform, "glGetActiveUniform", glsym_func_void);
   FALLBAK(_sym_glGetActiveUniform, glsym_func_void);

   FINDSYM(_sym_glGetAttachedShaders, "glGetAttachedShaders", glsym_func_void);
   FALLBAK(_sym_glGetAttachedShaders, glsym_func_void);

   FINDSYM(_sym_glGetAttribLocation, "glGetAttribLocation", glsym_func_int);
   FALLBAK(_sym_glGetAttribLocation, glsym_func_int);

   FINDSYM(_sym_glGetBooleanv, "glGetBooleanv", glsym_func_void);
   FALLBAK(_sym_glGetBooleanv, glsym_func_void);

   FINDSYM(_sym_glGetBufferParameteriv, "glGetBufferParameteriv", glsym_func_void);
   FALLBAK(_sym_glGetBufferParameteriv, glsym_func_void);

   FINDSYM(_sym_glGetError, "glGetError", glsym_func_uint);
   FALLBAK(_sym_glGetError, glsym_func_uint);

   FINDSYM(_sym_glGetFloatv, "glGetFloatv", glsym_func_void);
   FALLBAK(_sym_glGetFloatv, glsym_func_void);

   FINDSYM(_sym_glGetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv", glsym_func_void);
   FALLBAK(_sym_glGetFramebufferAttachmentParameteriv, glsym_func_void);

   FINDSYM(_sym_glGetIntegerv, "glGetIntegerv", glsym_func_void);
   FALLBAK(_sym_glGetIntegerv, glsym_func_void);

   FINDSYM(_sym_glGetProgramiv, "glGetProgramiv", glsym_func_void);
   FALLBAK(_sym_glGetProgramiv, glsym_func_void);

   FINDSYM(_sym_glGetProgramInfoLog, "glGetProgramInfoLog", glsym_func_void);
   FALLBAK(_sym_glGetProgramInfoLog, glsym_func_void);

   FINDSYM(_sym_glGetRenderbufferParameteriv, "glGetRenderbufferParameteriv", glsym_func_void);
   FALLBAK(_sym_glGetRenderbufferParameteriv, glsym_func_void);

   FINDSYM(_sym_glGetShaderiv, "glGetShaderiv", glsym_func_void);
   FALLBAK(_sym_glGetShaderiv, glsym_func_void);

   FINDSYM(_sym_glGetShaderInfoLog, "glGetShaderInfoLog", glsym_func_void);
   FALLBAK(_sym_glGetShaderInfoLog, glsym_func_void);

   FINDSYM(_sym_glGetShaderPrecisionFormat, "glGetShaderPrecisionFormat", glsym_func_void);
   FALLBAK(_sym_glGetShaderPrecisionFormat, glsym_func_void);

   FINDSYM(_sym_glGetShaderSource, "glGetShaderSource", glsym_func_void);
   FALLBAK(_sym_glGetShaderSource, glsym_func_void);

   FINDSYM(_sym_glGetString, "glGetString", glsym_func_uchar_ptr);
   FALLBAK(_sym_glGetString, glsym_func_const_uchar_ptr);

   FINDSYM(_sym_glGetTexParameterfv, "glGetTexParameterfv", glsym_func_void);
   FALLBAK(_sym_glGetTexParameterfv, glsym_func_void);

   FINDSYM(_sym_glGetTexParameteriv, "glGetTexParameteriv", glsym_func_void);
   FALLBAK(_sym_glGetTexParameteriv, glsym_func_void);

   FINDSYM(_sym_glGetUniformfv, "glGetUniformfv", glsym_func_void);
   FALLBAK(_sym_glGetUniformfv, glsym_func_void);

   FINDSYM(_sym_glGetUniformiv, "glGetUniformiv", glsym_func_void);
   FALLBAK(_sym_glGetUniformiv, glsym_func_void);

   FINDSYM(_sym_glGetUniformLocation, "glGetUniformLocation", glsym_func_int);
   FALLBAK(_sym_glGetUniformLocation, glsym_func_int);

   FINDSYM(_sym_glGetVertexAttribfv, "glGetVertexAttribfv", glsym_func_void);
   FALLBAK(_sym_glGetVertexAttribfv, glsym_func_void);

   FINDSYM(_sym_glGetVertexAttribiv, "glGetVertexAttribiv", glsym_func_void);
   FALLBAK(_sym_glGetVertexAttribiv, glsym_func_void);

   FINDSYM(_sym_glGetVertexAttribPointerv, "glGetVertexAttribPointerv", glsym_func_void);
   FALLBAK(_sym_glGetVertexAttribPointerv, glsym_func_void);

   FINDSYM(_sym_glHint, "glHint", glsym_func_void);
   FALLBAK(_sym_glHint, glsym_func_void);

   FINDSYM(_sym_glIsBuffer, "glIsBuffer", glsym_func_uchar);
   FALLBAK(_sym_glIsBuffer, glsym_func_uchar);

   FINDSYM(_sym_glIsEnabled, "glIsEnabled", glsym_func_uchar);
   FALLBAK(_sym_glIsEnabled, glsym_func_uchar);

   FINDSYM(_sym_glIsFramebuffer, "glIsFramebuffer", glsym_func_uchar);
   FALLBAK(_sym_glIsFramebuffer, glsym_func_uchar);

   FINDSYM(_sym_glIsProgram, "glIsProgram", glsym_func_uchar);
   FALLBAK(_sym_glIsProgram, glsym_func_uchar);

   FINDSYM(_sym_glIsRenderbuffer, "glIsRenderbuffer", glsym_func_uchar);
   FALLBAK(_sym_glIsRenderbuffer, glsym_func_uchar);

   FINDSYM(_sym_glIsShader, "glIsShader", glsym_func_uchar);
   FALLBAK(_sym_glIsShader, glsym_func_uchar);

   FINDSYM(_sym_glIsTexture, "glIsTexture", glsym_func_uchar);
   FALLBAK(_sym_glIsTexture, glsym_func_uchar);

   FINDSYM(_sym_glLineWidth, "glLineWidth", glsym_func_void);
   FALLBAK(_sym_glLineWidth, glsym_func_void);

   FINDSYM(_sym_glLinkProgram, "glLinkProgram", glsym_func_void);
   FALLBAK(_sym_glLinkProgram, glsym_func_void);

   FINDSYM(_sym_glPixelStorei, "glPixelStorei", glsym_func_void);
   FALLBAK(_sym_glPixelStorei, glsym_func_void);

   FINDSYM(_sym_glPolygonOffset, "glPolygonOffset", glsym_func_void);
   FALLBAK(_sym_glPolygonOffset, glsym_func_void);

   FINDSYM(_sym_glReadPixels, "glReadPixels", glsym_func_void);
   FALLBAK(_sym_glReadPixels, glsym_func_void);

   FINDSYM(_sym_glReleaseShaderCompiler, "glReleaseShaderCompiler", glsym_func_void);
   FALLBAK(_sym_glReleaseShaderCompiler, glsym_func_void);

   FINDSYM(_sym_glRenderbufferStorage, "glRenderbufferStorage", glsym_func_void);
   FALLBAK(_sym_glRenderbufferStorage, glsym_func_void);

   FINDSYM(_sym_glSampleCoverage, "glSampleCoverage", glsym_func_void);
   FALLBAK(_sym_glSampleCoverage, glsym_func_void);

   FINDSYM(_sym_glScissor, "glScissor", glsym_func_void);
   FALLBAK(_sym_glScissor, glsym_func_void);

   FINDSYM(_sym_glShaderBinary, "glShaderBinary", glsym_func_void);
   FALLBAK(_sym_glShaderBinary, glsym_func_void);

   FINDSYM(_sym_glShaderSource, "glShaderSource", glsym_func_void);
   FALLBAK(_sym_glShaderSource, glsym_func_void);

   FINDSYM(_sym_glStencilFunc, "glStencilFunc", glsym_func_void);
   FALLBAK(_sym_glStencilFunc, glsym_func_void);

   FINDSYM(_sym_glStencilFuncSeparate, "glStencilFuncSeparate", glsym_func_void);
   FALLBAK(_sym_glStencilFuncSeparate, glsym_func_void);

   FINDSYM(_sym_glStencilMask, "glStencilMask", glsym_func_void);
   FALLBAK(_sym_glStencilMask, glsym_func_void);

   FINDSYM(_sym_glStencilMaskSeparate, "glStencilMaskSeparate", glsym_func_void);
   FALLBAK(_sym_glStencilMaskSeparate, glsym_func_void);

   FINDSYM(_sym_glStencilOp, "glStencilOp", glsym_func_void);
   FALLBAK(_sym_glStencilOp, glsym_func_void);

   FINDSYM(_sym_glStencilOpSeparate, "glStencilOpSeparate", glsym_func_void);
   FALLBAK(_sym_glStencilOpSeparate, glsym_func_void);

   FINDSYM(_sym_glTexImage2D, "glTexImage2D", glsym_func_void);
   FALLBAK(_sym_glTexImage2D, glsym_func_void);

   FINDSYM(_sym_glTexParameterf, "glTexParameterf", glsym_func_void);
   FALLBAK(_sym_glTexParameterf, glsym_func_void);

   FINDSYM(_sym_glTexParameterfv, "glTexParameterfv", glsym_func_void);
   FALLBAK(_sym_glTexParameterfv, glsym_func_void);

   FINDSYM(_sym_glTexParameteri, "glTexParameteri", glsym_func_void);
   FALLBAK(_sym_glTexParameteri, glsym_func_void);

   FINDSYM(_sym_glTexParameteriv, "glTexParameteriv", glsym_func_void);
   FALLBAK(_sym_glTexParameteriv, glsym_func_void);

   FINDSYM(_sym_glTexSubImage2D, "glTexSubImage2D", glsym_func_void);
   FALLBAK(_sym_glTexSubImage2D, glsym_func_void);

   FINDSYM(_sym_glUniform1f, "glUniform1f", glsym_func_void);
   FALLBAK(_sym_glUniform1f, glsym_func_void);

   FINDSYM(_sym_glUniform1fv, "glUniform1fv", glsym_func_void);
   FALLBAK(_sym_glUniform1fv, glsym_func_void);

   FINDSYM(_sym_glUniform1i, "glUniform1i", glsym_func_void);
   FALLBAK(_sym_glUniform1i, glsym_func_void);

   FINDSYM(_sym_glUniform1iv, "glUniform1iv", glsym_func_void);
   FALLBAK(_sym_glUniform1iv, glsym_func_void);

   FINDSYM(_sym_glUniform2f, "glUniform2f", glsym_func_void);
   FALLBAK(_sym_glUniform2f, glsym_func_void);

   FINDSYM(_sym_glUniform2fv, "glUniform2fv", glsym_func_void);
   FALLBAK(_sym_glUniform2fv, glsym_func_void);

   FINDSYM(_sym_glUniform2i, "glUniform2i", glsym_func_void);
   FALLBAK(_sym_glUniform2i, glsym_func_void);

   FINDSYM(_sym_glUniform2iv, "glUniform2iv", glsym_func_void);
   FALLBAK(_sym_glUniform2iv, glsym_func_void);

   FINDSYM(_sym_glUniform3f, "glUniform3f", glsym_func_void);
   FALLBAK(_sym_glUniform3f, glsym_func_void);

   FINDSYM(_sym_glUniform3fv, "glUniform3fv", glsym_func_void);
   FALLBAK(_sym_glUniform3fv, glsym_func_void);

   FINDSYM(_sym_glUniform3i, "glUniform3i", glsym_func_void);
   FALLBAK(_sym_glUniform3i, glsym_func_void);

   FINDSYM(_sym_glUniform3iv, "glUniform3iv", glsym_func_void);
   FALLBAK(_sym_glUniform3iv, glsym_func_void);

   FINDSYM(_sym_glUniform4f, "glUniform4f", glsym_func_void);
   FALLBAK(_sym_glUniform4f, glsym_func_void);

   FINDSYM(_sym_glUniform4fv, "glUniform4fv", glsym_func_void);
   FALLBAK(_sym_glUniform4fv, glsym_func_void);

   FINDSYM(_sym_glUniform4i, "glUniform4i", glsym_func_void);
   FALLBAK(_sym_glUniform4i, glsym_func_void);

   FINDSYM(_sym_glUniform4iv, "glUniform4iv", glsym_func_void);
   FALLBAK(_sym_glUniform4iv, glsym_func_void);

   FINDSYM(_sym_glUniformMatrix2fv, "glUniformMatrix2fv", glsym_func_void);
   FALLBAK(_sym_glUniformMatrix2fv, glsym_func_void);

   FINDSYM(_sym_glUniformMatrix3fv, "glUniformMatrix3fv", glsym_func_void);
   FALLBAK(_sym_glUniformMatrix3fv, glsym_func_void);

   FINDSYM(_sym_glUniformMatrix4fv, "glUniformMatrix4fv", glsym_func_void);
   FALLBAK(_sym_glUniformMatrix4fv, glsym_func_void);

   FINDSYM(_sym_glUseProgram, "glUseProgram", glsym_func_void);
   FALLBAK(_sym_glUseProgram, glsym_func_void);

   FINDSYM(_sym_glValidateProgram, "glValidateProgram", glsym_func_void);
   FALLBAK(_sym_glValidateProgram, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib1f, "glVertexAttrib1f", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib1f, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib1fv, "glVertexAttrib1fv", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib1fv, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib2f, "glVertexAttrib2f", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib2f, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib2fv, "glVertexAttrib2fv", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib2fv, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib3f, "glVertexAttrib3f", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib3f, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib3fv, "glVertexAttrib3fv", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib3fv, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib4f, "glVertexAttrib4f", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib4f, glsym_func_void);

   FINDSYM(_sym_glVertexAttrib4fv, "glVertexAttrib4fv", glsym_func_void);
   FALLBAK(_sym_glVertexAttrib4fv, glsym_func_void);

   FINDSYM(_sym_glVertexAttribPointer, "glVertexAttribPointer", glsym_func_void);
   FALLBAK(_sym_glVertexAttribPointer, glsym_func_void);

   FINDSYM(_sym_glViewport, "glViewport", glsym_func_void);
   FALLBAK(_sym_glViewport, glsym_func_void);

#undef FINDSYM
#undef FALLBAK

   /*
    * For desktop OpenGL we wrap these:
    * - glGetShaderPrecisionFormat
    * - glReleaseShaderCompiler
    * - glShaderBinary
    */
   if (_sym_glGetShaderPrecisionFormat != (typeof(_sym_glGetShaderPrecisionFormat))sym_missing )
     {
        DBG("The GL library is OpenGL ES or OpenGL 4.1+");
        gl_lib_is_gles = 1;
     }

   if (!ok)
     ERR("Evas failed to initialize OSMesa for OpenGL with the software engine!");

   return ok;
}

//--------------------------------------------------------------//
// Wrapped GL APIs to handle desktop compatibility

// Stripping precision code from GLES shader for desktop compatibility
// Code adopted from Meego GL code. Temporary Fix.
/**
 * @brief Custom strtok-like function that handles C/C++ comments.
 * Used to parse shader source code while skipping comments.
 * @param s Input string (or NULL to continue tokenizing).
 * @param n Pointer to remaining length of the input string.
 * @param saveptr Pointer to store the internal state for subsequent calls.
 * @param prevbuf The buffer returned by the previous call (will be freed).
 * @return A newly allocated string containing the next token, or NULL if no more tokens.
 */
static const char *
opengl_strtok(const char *s, int *n, char **saveptr, char *prevbuf)
{
   char *start;
   char *ret;
   char *p;
   int retlen;
   static const char *delim = " \t\n\r/";

   if (prevbuf) free(prevbuf);

   if (s)
      *saveptr = (char *)s;
   else
     {
        if (!(*saveptr) || !(*n))
           return NULL;
        s = *saveptr;
     }

   for (; *n && strchr(delim, *s); s++, (*n)--)
     {
        if (*s == '/' && *n > 1)
          {
             if (s[1] == '/')
               {
                  do
                    {
                       s++, (*n)--;
                    }
                  while (*n > 1 && s[1] != '\n' && s[1] != '\r');
               }
             else if (s[1] == '*')
               {
                  do
                    {
                       s++, (*n)--;
                    }
                  while (*n > 2 && (s[1] != '*' || s[2] != '/'));
                  s++, (*n)--;
               }
          }
     }

   start = (char *)s;
   for (; *n && *s && !strchr(delim, *s); s++, (*n)--);
   if (*n > 0) s++, (*n)--;

   *saveptr = (char *)s;

   retlen = s - start;
   ret = malloc(retlen + 1);
   p = ret;

   while (retlen > 0)
     {
        if (*start == '/' && retlen > 1)
          {
             if (start[1] == '/')
               {
                  do
                    {
                       start++, retlen--;
                    }
                  while (retlen > 1 && start[1] != '\n' && start[1] != '\r');
                  start++, retlen--;
                  continue;
               }
             else if (start[1] == '*')
               {
                  do
                    {
                       start++, retlen--;
                    }
                  while (retlen > 2 && (start[1] != '*' || start[2] != '/'));
                  start += 3, retlen -= 3;
                  continue;
               }
          }
        *(p++) = *(start++), retlen--;
     }

   *p = 0;
   return ret;
}

/**
 * @brief Patches GLES shader source code for compatibility with desktop OpenGL.
 * Removes precision qualifiers (lowp, mediump, highp) and replaces GLES-specific
 * built-in variables (like gl_MaxVertexUniformVectors) with their desktop GL equivalents.
 * Uses opengl_strtok to handle comments correctly.
 * @param source The original GLES shader source code.
 * @param length The length of the source code.
 * @param patched_len Pointer to store the length of the patched shader code.
 * @return A newly allocated string containing the patched shader source, or NULL on failure.
 */
static char *
patch_gles_shader(const char *source, int length, int *patched_len)
{
   char *saveptr = NULL;
   char *sp;
   char *p = NULL;

   if (!length) length = strlen(source);

   *patched_len = 0;
   int patched_size = length;
   char *patched = malloc(patched_size + 1);

   if (!patched) return NULL;

   p = (char *)opengl_strtok(source, &length, &saveptr, NULL);
   for (; p; p = (char *)opengl_strtok(0, &length, &saveptr, p))
     {
        if (!strncmp(p, "lowp", 4) || !strncmp(p, "mediump", 7) || !strncmp(p, "highp", 5))
          {
             continue;
          }
        else if (!strncmp(p, "precision", 9))
          {
             while ((p = (char *)opengl_strtok(0, &length, &saveptr, p)) && !strchr(p, ';'));
          }
        else
          {
             if (!strncmp(p, "gl_MaxVertexUniformVectors", 26))
               {
                  free(p);
                  p = strdup("(gl_MaxVertexUniformComponents / 4)");
               }
             else if (!strncmp(p, "gl_MaxFragmentUniformVectors", 28))
               {
                  free(p);
                  p = strdup("(gl_MaxFragmentUniformComponents / 4)");
               }
             else if (!strncmp(p, "gl_MaxVaryingVectors", 20))
               {
                  free(p);
                  p = strdup("(gl_MaxVaryingFloats / 4)");
               }

             int new_len = strlen(p);
             if (*patched_len + new_len > patched_size)
               {
                  char *tmp;

                  patched_size *= 2;
                  tmp = realloc(patched, patched_size + 1);
                  if (!tmp)
                    {
                       free(patched);
                       free(p);
                       return NULL;
                    }
                  patched = tmp;
               }

             memcpy(patched + *patched_len, p, new_len);
             *patched_len += new_len;
          }
     }

   patched[*patched_len] = 0;
   /* check that we don't leave dummy preprocessor lines */
   for (sp = patched; *sp;)
     {
        for (; *sp == ' ' || *sp == '\t'; sp++);
        if (!strncmp(sp, "#define", 7))
          {
             for (p = sp + 7; *p == ' ' || *p == '\t'; p++);
             if (*p == '\n' || *p == '\r' || *p == '/')
               {
                  memset(sp, 0x20, 7);
               }
          }
        for (; *sp && *sp != '\n' && *sp != '\r'; sp++);
        for (; *sp == '\n' || *sp == '\r'; sp++);
     }
   return patched;
}

/**
 * @brief Wrapper for glShaderSource that patches GLES shaders for desktop GL compatibility.
 * Calls patch_gles_shader before passing the source to the real _sym_glShaderSource.
 * @param shader The shader object handle.
 * @param count The number of strings in the source array.
 * @param string Array of source code strings.
 * @param length Array of string lengths (or NULL for null-terminated strings).
 */
static void
evgl_glShaderSource(GLuint shader, GLsizei count, const char* const* string, const GLint* length)
{
   int i = 0, len = 0;

   char **s = malloc(count * sizeof(char*));
   if (!s) goto err;
   GLint *l = malloc(count * sizeof(GLint));
   if (!l)
     {
        free(s);
        goto err;
     }

   memset(s, 0, count * sizeof(char*));
   memset(l, 0, count * sizeof(GLint));

   for (i = 0; i < count; ++i)
     {
        if (length)
          {
             len = length[i];
             if (len < 0)
                len = string[i] ? strlen(string[i]) : 0;
          }
        else
           len = string[i] ? strlen(string[i]) : 0;

        if (string[i])
          {
             s[i] = patch_gles_shader(string[i], len, &l[i]);
             if (!s[i])
               {
                  while(i)
                     free(s[--i]);
                  free(l);
                  free(s);
                  goto err;
               }
          }
        else
          {
             s[i] = NULL;
             l[i] = 0;
          }
     }

   _sym_glShaderSource(shader, count, (const char * const *)s, l);

   while(i)
      free(s[--i]);
   free(l);
   free(s);

err:
   ERR("Patching Shader Failed.");
}


/**
 * @brief Wrapper/Emulation for glGetShaderPrecisionFormat for desktop GL.
 * Provides fixed precision/range values typical for desktop float precision,
 * as desktop GL prior to 4.1 doesn't have this function.
 * @param shadertype Shader type (unused).
 * @param precisiontype Precision type (unused).
 * @param range Pointer to store the range [min, max].
 * @param precision Pointer to store the precision bits.
 */
static void
evgl_glGetShaderPrecisionFormat(GLenum shadertype EINA_UNUSED, GLenum precisiontype EINA_UNUSED, GLint* range, GLint* precision)
{
   if (range)
     {
        range[0] = -126; // floor(log2(FLT_MIN))
        range[1] = 127; // floor(log2(FLT_MAX))
     }
   if (precision)
     {
        precision[0] = 24; // floor(-log2((1.0/16777218.0)));
     }
   return;
}

/**
 * @brief Wrapper/Emulation for glReleaseShaderCompiler for desktop GL.
 * This is a no-op on desktop GL as shader compilation is typically synchronous.
 */
static void
evgl_glReleaseShaderCompiler(void)
{
   DBG("Not supported in Desktop GL");
   return;
}

/**
 * @brief Wrapper/Emulation for glShaderBinary for desktop GL.
 * This is generally not supported on desktop GL in the same way as GLES. Logs a debug message.
 * @param n Number of shaders (unused).
 * @param shaders Array of shader handles (unused).
 * @param binaryformat Binary format enum (unused).
 * @param binary Pointer to binary data (unused).
 * @param length Length of binary data (unused).
 */
static void
evgl_glShaderBinary(GLsizei n EINA_UNUSED, const GLuint* shaders EINA_UNUSED, GLenum binaryformat EINA_UNUSED, const void* binary EINA_UNUSED, GLsizei length EINA_UNUSED)
{
   // FIXME: need to dlsym/getprocaddress for this
   DBG("Not supported in Desktop GL");
   return;
   //n = binaryformat = length = 0;
   //shaders = binary = 0;
}

/**
 * @brief Wrapper for glGetString to modify version strings for GLES compatibility.
 * Returns modified strings for GL_VERSION and GL_SHADING_LANGUAGE_VERSION to
 * report as GLES 2.0 / GLSL ES 1.00, while embedding the original desktop GL version.
 * Passes through other string queries (VENDOR, RENDERER, EXTENSIONS) directly.
 * @param name The GLenum specifying the string to query.
 * @return Pointer to the (potentially modified) GL string.
 */
static const GLubyte *
evgl_glGetString(GLenum name)
{
   static char _version[128] = {0};
   static char _glsl[128] = {0};
   const char *ret;

   /* NOTE: Please modify gl_common/evas_gl_api.c as well if you change
    *       this function!
    */

   switch (name)
     {
      case GL_VENDOR:
      case GL_RENDERER:
        // Keep these as-is.
        break;

      case GL_SHADING_LANGUAGE_VERSION:
        ret = (const char *) _sym_glGetString(GL_SHADING_LANGUAGE_VERSION);
        if (!ret) return NULL;
        snprintf(_glsl, sizeof(_glsl), "OpenGL ES GLSL ES 1.00 Evas GL (%s)", (char *) ret);
        _version[sizeof(_glsl) - 1] = '\0';
        return (const GLubyte *) _glsl;

      case GL_VERSION:
        ret = (const char *) _sym_glGetString(GL_VERSION);
        if (!ret) return NULL;
        snprintf(_version, sizeof(_version), "OpenGL ES 2.0 Evas GL (%s)", (char *) ret);
        _version[sizeof(_version) - 1] = '\0';
        return (const GLubyte *) _version;

      case GL_EXTENSIONS:
        // assume OSMesa's extensions are safe (no messing with GL context here)
        break;

      default:
        // GL_INVALID_ENUM is generated if name is not an accepted value.
        WRN("Unknown string requested: %x", (unsigned int) name);
        break;
     }

   return _sym_glGetString(name);
}


/**
 * @brief Populates the Evas_GL_API structure with function pointers.
 * Assigns the resolved symbols (_sym_*) to the corresponding fields in the API struct.
 * Overrides specific functions with wrappers (evgl_*) for desktop GL compatibility if needed.
 * @param api Pointer to the Evas_GL_API structure to populate.
 */
static void
override_gl_apis(Evas_GL_API *api)
{
   memset(api, 0, sizeof(*api));
   api->version = EVAS_GL_API_VERSION;

#define ORD(f) EVAS_API_OVERRIDE(f, api, _sym_)
   // GLES 2.0
   ORD(glActiveTexture);
   ORD(glAttachShader);
   ORD(glBindAttribLocation);
   ORD(glBindBuffer);
   ORD(glBindFramebuffer);
   ORD(glBindRenderbuffer);
   ORD(glBindTexture);
   ORD(glBlendColor);
   ORD(glBlendEquation);
   ORD(glBlendEquationSeparate);
   ORD(glBlendFunc);
   ORD(glBlendFuncSeparate);
   ORD(glBufferData);
   ORD(glBufferSubData);
   ORD(glCheckFramebufferStatus);
   ORD(glClear);
   ORD(glClearColor);
   ORD(glClearDepthf);
   ORD(glClearStencil);
   ORD(glColorMask);
   ORD(glCompileShader);
   ORD(glCompressedTexImage2D);
   ORD(glCompressedTexSubImage2D);
   ORD(glCopyTexImage2D);
   ORD(glCopyTexSubImage2D);
   ORD(glCreateProgram);
   ORD(glCreateShader);
   ORD(glCullFace);
   ORD(glDeleteBuffers);
   ORD(glDeleteFramebuffers);
   ORD(glDeleteProgram);
   ORD(glDeleteRenderbuffers);
   ORD(glDeleteShader);
   ORD(glDeleteTextures);
   ORD(glDepthFunc);
   ORD(glDepthMask);
   ORD(glDepthRangef);
   ORD(glDetachShader);
   ORD(glDisable);
   ORD(glDisableVertexAttribArray);
   ORD(glDrawArrays);
   ORD(glDrawElements);
   ORD(glEnable);
   ORD(glEnableVertexAttribArray);
   ORD(glFinish);
   ORD(glFlush);
   ORD(glFramebufferRenderbuffer);
   ORD(glFramebufferTexture2D);
   ORD(glFrontFace);
   ORD(glGenBuffers);
   ORD(glGenerateMipmap);
   ORD(glGenFramebuffers);
   ORD(glGenRenderbuffers);
   ORD(glGenTextures);
   ORD(glGetActiveAttrib);
   ORD(glGetActiveUniform);
   ORD(glGetAttachedShaders);
   ORD(glGetAttribLocation);
   ORD(glGetBooleanv);
   ORD(glGetBufferParameteriv);
   ORD(glGetError); // FIXME
   ORD(glGetFloatv);
   ORD(glGetFramebufferAttachmentParameteriv);
   ORD(glGetIntegerv);
   ORD(glGetProgramiv);
   ORD(glGetProgramInfoLog);
   ORD(glGetRenderbufferParameteriv);
   ORD(glGetShaderiv);
   ORD(glGetShaderInfoLog);
   ORD(glGetShaderPrecisionFormat);
   ORD(glGetShaderSource);
   ORD(glGetTexParameterfv);
   ORD(glGetTexParameteriv);
   ORD(glGetUniformfv);
   ORD(glGetUniformiv);
   ORD(glGetUniformLocation);
   ORD(glGetVertexAttribfv);
   ORD(glGetVertexAttribiv);
   ORD(glGetVertexAttribPointerv);
   ORD(glHint);
   ORD(glIsBuffer);
   ORD(glIsEnabled);
   ORD(glIsFramebuffer);
   ORD(glIsProgram);
   ORD(glIsRenderbuffer);
   ORD(glIsShader);
   ORD(glIsTexture);
   ORD(glLineWidth);
   ORD(glLinkProgram);
   ORD(glPixelStorei);
   ORD(glPolygonOffset);
   ORD(glReadPixels);
   ORD(glReleaseShaderCompiler);
   ORD(glRenderbufferStorage);
   ORD(glSampleCoverage);
   ORD(glScissor);
   ORD(glShaderBinary);
   ORD(glShaderSource);
   ORD(glStencilFunc);
   ORD(glStencilFuncSeparate);
   ORD(glStencilMask);
   ORD(glStencilMaskSeparate);
   ORD(glStencilOp);
   ORD(glStencilOpSeparate);
   ORD(glTexImage2D);
   ORD(glTexParameterf);
   ORD(glTexParameterfv);
   ORD(glTexParameteri);
   ORD(glTexParameteriv);
   ORD(glTexSubImage2D);
   ORD(glUniform1f);
   ORD(glUniform1fv);
   ORD(glUniform1i);
   ORD(glUniform1iv);
   ORD(glUniform2f);
   ORD(glUniform2fv);
   ORD(glUniform2i);
   ORD(glUniform2iv);
   ORD(glUniform3f);
   ORD(glUniform3fv);
   ORD(glUniform3i);
   ORD(glUniform3iv);
   ORD(glUniform4f);
   ORD(glUniform4fv);
   ORD(glUniform4i);
   ORD(glUniform4iv);
   ORD(glUniformMatrix2fv);
   ORD(glUniformMatrix3fv);
   ORD(glUniformMatrix4fv);
   ORD(glUseProgram);
   ORD(glValidateProgram);
   ORD(glVertexAttrib1f);
   ORD(glVertexAttrib1fv);
   ORD(glVertexAttrib2f);
   ORD(glVertexAttrib2fv);
   ORD(glVertexAttrib3f);
   ORD(glVertexAttrib3fv);
   ORD(glVertexAttrib4f);
   ORD(glVertexAttrib4fv);
   ORD(glVertexAttribPointer);
   ORD(glViewport);
#undef ORD

#define ORD(f) EVAS_API_OVERRIDE(f, &gl_funcs, evgl_)
   ORD(glGetString);

   if (!gl_lib_is_gles)
     {
        // Override functions wrapped by Evas_GL
        // GLES2.0 API compat on top of desktop gl
        // Note that Open GL 4.1+ provides these 3 functions as well
        ORD(glGetShaderPrecisionFormat);
        ORD(glReleaseShaderCompiler);
        ORD(glShaderBinary);
     }

   ORD(glShaderSource);    // Do precision stripping in both cases
#undef ORD
}
#endif

//-------------------------------------------//
/**
 * @brief Initializes the GL library (OSMesa) support for the engine.
 * Attempts to dlopen libOSMesa, resolves core OSMesa symbols, resolves GL API symbols,
 * and sets up the Evas_GL_API function table with potential compatibility wrappers.
 * Ensures thread-local storage is initialized.
 * @return 1 on successful initialization, 0 on failure (OSMesa not found or symbol resolution failed).
 */
static int
gl_lib_init(void)
{
#ifdef EVAS_GL
   // Current ctx & sfc stuff
   if (gl_lib_handle) return 1;
   if (!_tls_check()) return 0;

   // dlopen OSMesa
   gl_lib_handle = dlopen("libOSMesa.so.9", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.8", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.7", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.6", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.5", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.4", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.3", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.2", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so.1", RTLD_NOW);
   if (!gl_lib_handle) gl_lib_handle = dlopen("libOSMesa.so", RTLD_NOW);
   if (!gl_lib_handle)
     {
        WRN("Unable to open libOSMesa:  %s", dlerror());
        DBG("Unable to support EvasGL in this engine module. Install OSMesa to get it running");
        return 0;
     }

   //------------------------------------------------//
   if (!glue_sym_init())
     {
        WRN("Unable to glue OSMesa syms");
        return 0;
     }
   if (!gl_sym_init())
     {
        WRN("Unable to init OSMesa syms");
        return 0;
     }

   override_gl_apis(&gl_funcs);

   return 1;
#else
   WRN("Evas GL not compiled in");
   return 0;
#endif
}

/*
static void
init_gl(void)
{
   DBG("Initializing Software OpenGL APIs...");

   if (!gl_lib_init())
      DBG("Unable to support EvasGL in this engine module. Install OSMesa to get it running");
   else
     {
#define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
        ORD(gl_surface_create);
        ORD(gl_surface_destroy);
        ORD(gl_context_create);
        ORD(gl_context_destroy);
        ORD(gl_make_current);
        ORD(gl_string_query);           // FIXME: Need to implement
        ORD(gl_proc_address_get);       // FIXME: Need to implement
        ORD(gl_native_surface_get);
        ORD(gl_error_get);
        ORD(gl_current_context_get);
        ORD(gl_current_surface_get);
        ORD(gl_rotation_angle_get);
#undef ORD
     }
}
*/

/*
 *****
 **
 ** MODULE ACCESSIBLE API API
 **
 *****
 */

/**
 * @brief Opens and initializes the software_generic engine module.
 * Registers the log domain, creates mempools for threaded commands, initializes
 * Ector and pipe rendering subsystems, and registers the engine's function table.
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   _evas_soft_gen_log_dom = eina_log_domain_register
     ("evas-software_generic", EVAS_DEFAULT_LOG_COLOR);
   if(_evas_soft_gen_log_dom<0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   _mp_command_rect = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Rect", NULL,
                                       sizeof (Evas_Thread_Command_Rect), 128);
   _mp_command_line = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Line", NULL,
                                       sizeof (Evas_Thread_Command_Line), 32);
   _mp_command_polygon = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Polygon", NULL,
                                       sizeof (Evas_Thread_Command_Polygon), 32);
   _mp_command_image = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Image", NULL,
                                       sizeof (Evas_Thread_Command_Image), 128);
   _mp_command_font = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Font", NULL,
                                       sizeof (Evas_Thread_Command_Font), 128);
   _mp_command_map = eina_mempool_add("chained_mempool",
                                       "Evas_Thread_Command_Map", NULL,
                                       sizeof (Evas_Thread_Command_Map), 64);
   _mp_command_multi_font =
     eina_mempool_add("chained_mempool", "Evas_Thread_Command_Multi_Font",
                      NULL, sizeof(Evas_Thread_Command_Multi_Font), 128);
   _mp_command_ector =
     eina_mempool_add("chained_mempool", "Evas_Thread_Command_Ector",
                      NULL, sizeof(Evas_Thread_Command_Ector), 128);
   _mp_command_ector_surface =
     eina_mempool_add("chained_mempool", "Evas_Thread_Command_Ector_Surface",
                      NULL, sizeof(Evas_Thread_Command_Ector_Surface), 128);

   ector_init();
// do on demand when first evas_gl_api_get is called...
//   init_gl();
   ector_glsym_set(dlsym, RTLD_DEFAULT);
   evas_common_pipe_init();

   em->functions = (void *)(&func);
   cpunum = eina_cpu_count();
   return 1;
}

/**
 * @brief Closes and cleans up the software_generic engine module.
 * Shuts down Ector, deletes command mempools, and unregisters the log domain.
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   ector_shutdown();
   eina_mempool_del(_mp_command_rect);
   eina_mempool_del(_mp_command_line);
   eina_mempool_del(_mp_command_polygon);
   eina_mempool_del(_mp_command_image);
   eina_mempool_del(_mp_command_font);
   eina_mempool_del(_mp_command_map);
   eina_mempool_del(_mp_command_ector);
   if (_evas_soft_gen_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_soft_gen_log_dom);
        _evas_soft_gen_log_dom = -1;
     }
}

/**
 * @brief Module API structure defining the engine module.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "software_generic",
   "none",
   {
     module_open,
     module_close
   }
};

/**
 * @brief Initializes and registers the software_generic engine module with Evas.
 * Called by Evas during module loading.
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise.
 */
Eina_Bool evas_engine_software_generic_init(void)
{
   return evas_module_register(&evas_modapi, EVAS_MODULE_TYPE_ENGINE);
}

/**
 * @brief Unregisters the software_generic engine module from Evas.
 * Called by Evas during module unloading.
 */
// Time to destroy the ector context
void evas_engine_software_generic_shutdown(void)
{
   evas_module_unregister(&evas_modapi, EVAS_MODULE_TYPE_ENGINE);
}

#ifndef EVAS_STATIC_BUILD_SOFTWARE_GENERIC
EVAS_EINA_MODULE_DEFINE(engine, software_generic);
#endif
