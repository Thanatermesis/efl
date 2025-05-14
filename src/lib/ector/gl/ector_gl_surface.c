#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Eet.h>
#include <Ector.h>

#include "gl/Ector_GL.h"
#include "ector_private.h"
#include "ector_gl_private.h"

/**
 * @brief Private data for the Ector_GL_Surface class.
 *
 * This structure holds data specific to an Ector GL surface instance,
 * including the reference point for drawing operations and the current
 * rendering operation.
 */
typedef struct _Ector_GL_Surface_Data Ector_GL_Surface_Data;
struct _Ector_GL_Surface_Data
{
   struct {
      int x, y; /**< The x and y coordinates of the reference point. */
   } reference_point; /**< Reference point for drawing operations. Typically the top-left corner of a buffer. */

   Efl_Gfx_Render_Op op; /**< Current graphics rendering operation (e.g., blend, copy). */
};

/**
 * @brief Represents a cached shader program.
 *
 * This structure stores a compiled GL shader program along with the flags
 * that were used to generate or identify it. This allows for efficient
 * reuse of shaders.
 */
typedef struct _Ector_Shader Ector_Shader;
struct _Ector_Shader
{
   uint64_t flags; /**< Flags identifying the shader's capabilities and configuration. */
   GLuint prg;     /**< The OpenGL shader program ID. */
};

static Eina_Hash *shader_cache = NULL; /**< Hash table for caching compiled Ector_Shader objects. Keyed by shader flags. */
static Eet_File *shader_file = NULL;   /**< Eet file handle for storing and retrieving pre-compiled shader binaries. */

/**
 * @brief Frees an Ector_Shader object.
 *
 * This function is used as a callback for the shader_cache hash table to
 * clean up Ector_Shader instances when they are removed from the cache.
 * It deletes the GL program and frees the structure memory.
 *
 * @param s Pointer to the Ector_Shader data to be freed.
 */
static void
_shader_free(void *s)
{
   Ector_Shader *shd = s;

   GL.glDeleteProgram(shd->prg);
   free(shd);
}

/**
 * @brief Factory function to create renderer objects for the GL surface.
 *
 * Based on the requested renderer type (e.g., shape, linear gradient, radial gradient),
 * this function instantiates and returns the appropriate GL-specific renderer.
 * The created renderer is associated with the given Ector surface.
 *
 * @param obj The Ector_GL_Surface object.
 * @param pd Private data of the Ector_GL_Surface object (unused in this function).
 * @param type The Efl_Class of the renderer mixin to be created (e.g., ECTOR_RENDERER_SHAPE_MIXIN).
 * @return A new Ector_Renderer instance of the specified type, or NULL if the type is not supported.
 *         The caller is responsible for decrementing the reference count of the returned object
 *         when it's no longer needed, as efl_add_ref is used.
 */
static Ector_Renderer *
_ector_gl_surface_ector_surface_renderer_factory_new(Eo *obj,
                                                             Ector_GL_Surface_Data *pd EINA_UNUSED,
                                                             const Efl_Class *type)
{
   if (type == ECTOR_RENDERER_SHAPE_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_GL_SHAPE_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));
   else if (type == ECTOR_RENDERER_GRADIENT_LINEAR_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_GL_GRADIENT_LINEAR_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));
   else if (type == ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));

   ERR("Couldn't find class for type: %s\n", efl_class_name_get(type));
   return NULL;
}

/**
 * @brief Sets the reference point for drawing operations on the GL surface.
 *
 * The reference point is typically the origin (e.g., top-left corner) against which
 * drawing coordinates are interpreted.
 *
 * @param obj The Ector_GL_Surface object (unused in this function).
 * @param pd Private data of the Ector_GL_Surface object.
 * @param x The x-coordinate of the reference point.
 * @param y The y-coordinate of the reference point.
 */
static void
_ector_gl_surface_ector_surface_reference_point_set(Eo *obj EINA_UNUSED,
                                                            Ector_GL_Surface_Data *pd,
                                                            int x, int y)
{
   pd->reference_point.x = x;
   pd->reference_point.y = y;
}

#define VERTEX_CNT 3 /**< Number of components per vertex (e.g., x, y, z). */
#define COLOR_CNT 4  /**< Number of components per color (e.g., r, g, b, a). */

/**
 * @brief Pushes vertex data to the GPU for rendering.
 *
 * This function selects an appropriate shader based on the provided flags,
 * sets up the GL state (vertex attributes, uniforms), and issues a draw call.
 *
 * @param obj The Ector_GL_Surface object.
 * @param pd Private data of the Ector_GL_Surface object (unused in this function).
 * @param flags Shader flags used to select or compile the appropriate shader program.
 *              These flags define features like texturing, color modes, etc.
 * @param vertex Pointer to an array of vertex coordinates. Each vertex consists of VERTEX_CNT GLshort values.
 *               Example for one triangle: {x1, y1, z1,  x2, y2, z2,  x3, y3, z3}
 * @param vertex_count The number of vertices to draw. For GL_TRIANGLES, this should be a multiple of 3.
 * @param mul_col A packed unsigned integer representing the multiplication color (e.g., 0xAARRGGBB).
 *                This color is applied uniformly to all vertices.
 * @return EINA_TRUE on success, EINA_FALSE on failure (though currently always returns EINA_TRUE).
 */
static Eina_Bool
_ector_gl_surface_push(Eo *obj,
                       Ector_GL_Surface_Data *pd EINA_UNUSED,
                       uint64_t flags, GLshort *vertex, unsigned int vertex_count, unsigned int mul_col)
{
   unsigned int prog;

   prog = ector_gl_surface_shader_get(obj, flags);

   // FIXME: Not using map/unmap buffer yet, nor any pipe
   // FIXME: Move some of the state change to start of surface drawing?
   GL.glUseProgram(prog);
   GL.glDisable(GL_TEXTURE_2D);
   GL.glDisable(GL_SCISSOR_TEST);
   GL.glVertexAttribPointer(SHAD_VERTEX, VERTEX_CNT, GL_SHORT, GL_FALSE, 0, vertex);
   GL.glEnableVertexAttribArray(SHAD_COLOR);
   GL.glVertexAttribPointer(SHAD_COLOR, COLOR_CNT, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mul_col);
   GL.glDrawArrays(GL_TRIANGLES, 0, vertex_count);

   return EINA_TRUE;
}

/**
 * @brief Defines the rendering state, such as blend operation and clipping.
 *
 * This function sets up the GL state according to the specified rendering operation
 * (e.g., blend, copy) and applies clipping regions.
 *
 * @param obj The Ector_GL_Surface object (unused in this function).
 * @param pd Private data of the Ector_GL_Surface object.
 * @param op The rendering operation to apply (e.g., EFL_GFX_RENDER_OP_BLEND, EFL_GFX_RENDER_OP_COPY).
 * @param clips An Eina_Array of Eina_Rect structures defining the clipping regions.
 *              Currently, clipping is not fully implemented (FIXME).
 *              Example of clips array structure:
 *              Eina_Array *clips = eina_array_new(1);
 *              Eina_Rect clip_rect = { .x = 10, .y = 10, .w = 100, .h = 100 };
 *              eina_array_push(clips, &clip_rect);
 * @return EINA_TRUE if the state was successfully set, EINA_FALSE if the operation is invalid.
 */
static Eina_Bool
_ector_gl_surface_state_define(Eo *obj EINA_UNUSED, Ector_GL_Surface_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips)
{
   if (pd->op == op) return EINA_TRUE;

   // FIXME: no pipe yet, so we can just change the mode right away
   // Get & apply matrix transformation too
   switch (op)
     {
      case EFL_GFX_RENDER_OP_BLEND: /**< default op: d = d*(1-sa) + s */
         GL.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
         GL.glEnable(GL_BLEND);
         break;
      case EFL_GFX_RENDER_OP_COPY: /**< d = s */
         // Just disable blend mode. no need to set blend func
         GL.glDisable(GL_BLEND);
         break;
      case EFL_GFX_RENDER_OP_LAST:
      default:
         return EINA_FALSE;
     }

   pd->op = op;

   // FIXME: we should not ignore clipping, but that can last for later
   (void) clips;

   return EINA_TRUE;
}

/**
 * @brief Binds texture units for a given shader program.
 *
 * This function inspects the flags of the provided Ector_Shader to determine
 * which texture samplers are active (e.g., main texture, mask texture, YUV planes).
 * It then binds these samplers to sequential texture units (GL_TEXTURE0, GL_TEXTURE1, etc.)
 * by setting their corresponding uniform locations in the shader program.
 *
 * @param p Pointer to the Ector_Shader whose textures need to be bound.
 *          If NULL, the function does nothing.
 */
static void
_ector_gl_shader_textures_bind(Ector_Shader *p)
{
   struct {
      const char *name;
      int enabled;
   } textures[] = {
      { "tex", 0 },
      { "texm", 0 },
      { "texa", 0 },
      { "texu", 0 },
      { "texv", 0 },
      { "texuv", 0 },
      { NULL, 0 }
   };
   Eina_Bool hastex = 0;
   int tex_count = 0;
   GLint loc;
   int i;

   if (!p) return;

   if ((p->flags & SHADER_FLAG_TEX) != 0)
     {
        textures[0].enabled = 1;
        hastex = 1;
     }
   if ((p->flags & SHADER_FLAG_MASK) != 0)
     {
        textures[1].enabled = 1;
        hastex = 1;
     }
   if ((p->flags & SHADER_FLAG_RGB_A_PAIR) != 0)
     {
        textures[2].enabled = 1;
        hastex = 1;
     }
   if (p->flags & SHADER_FLAG_YUV)
     {
        textures[3].enabled = 1;
        textures[4].enabled = 1;
        hastex = 1;
     }
   else if ((p->flags & SHADER_FLAG_NV12) || (p->flags & SHADER_FLAG_YUY2))
     {
        textures[5].enabled = 1;
        hastex = 1;
     }

   if (hastex)
     {
        GL.glUseProgram(p->prg); // FIXME: is this necessary??
        for (i = 0; textures[i].name; i++)
          {
             if (!textures[i].enabled) continue;
             loc = GL.glGetUniformLocation(p->prg, textures[i].name);
             if (loc < 0)
               {
                  ERR("Couldn't find uniform '%s' (shader: %16" PRIx64 ")",
                      textures[i].name, p->flags);
               }
             GL.glUniform1i(loc, tex_count++);
          }
     }
}

/**
 * @brief Loads a pre-compiled shader binary from the Eet cache file.
 *
 * This function attempts to read a shader program that was previously compiled
 * and stored in an Eet file. The shader is identified by its flags.
 * If successful, it creates an Ector_Shader object, configures it, and binds
 * its texture samplers.
 *
 * @param flags A bitmask of flags that uniquely identifies the shader program.
 *              These flags determine the features and configuration of the shader.
 * @return A pointer to a newly allocated Ector_Shader object if the shader was
 *         successfully loaded and configured. Returns NULL on failure (e.g., shader
 *         not found in cache, GL error during program binary loading).
 *         The caller is responsible for freeing the returned Ector_Shader.
 */
static Ector_Shader *
_ector_gl_shader_load(uint64_t flags)
{
   Eina_Strbuf *buf;
   Ector_Shader *r = NULL;
   void *data;
   int *formats = NULL;
   int length = 0, num = 0;
   GLuint prg;
   GLint ok = 0, vtx = GL_NONE, frg = GL_NONE;
   Eina_Bool direct = 1;

   buf = eina_strbuf_new();
   if (!buf) return NULL;

   eina_strbuf_append_printf(buf, "ector/shader/%16" PRIx64, flags);

   data = (void*) eet_read_direct(shader_file, eina_strbuf_string_get(buf), &length);
   if (!data)
     {
        data = eet_read(shader_file, eina_strbuf_string_get(buf), &length);
        direct = 0;
     }
   if ((!data) || (length <= 0)) goto on_error;

   GL.glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num);
   if (num <= 0) goto on_error;

   formats = calloc(num, sizeof(int));
   if (!formats) goto on_error;

   GL.glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats);
   if (!formats[0]) goto on_error;

   prg = GL.glCreateProgram();
   // TODO: invalid rendering error occurs when attempting to use a
   // glProgramBinary.  In order to render correctly, we should create a
   // dummy vertex shader.
   vtx = GL.glCreateShader(GL_VERTEX_SHADER);
   GL.glAttachShader(prg, vtx);
   frg = GL.glCreateShader(GL_FRAGMENT_SHADER);
   GL.glAttachShader(prg, frg);

   GL.glProgramBinary(prg, formats[0], data, length);

   GL.glBindAttribLocation(prg, SHAD_VERTEX,  "vertex");
   GL.glBindAttribLocation(prg, SHAD_COLOR,   "color");
   GL.glBindAttribLocation(prg, SHAD_TEXUV,   "tex_coord");
   GL.glBindAttribLocation(prg, SHAD_TEXUV2,  "tex_coord2");
   GL.glBindAttribLocation(prg, SHAD_TEXUV3,  "tex_coord3");
   GL.glBindAttribLocation(prg, SHAD_TEXA,    "tex_coorda");
   GL.glBindAttribLocation(prg, SHAD_TEXSAM,  "tex_sample");
   GL.glBindAttribLocation(prg, SHAD_MASK,    "mask_coord");
   GL.glBindAttribLocation(prg, SHAD_MASKSAM, "tex_masksample");

   GL.glGetProgramiv(prg, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(prg, "load a program object", EINA_FALSE);
        ERR("Abort load of program (%s)", eina_strbuf_string_get(buf));
        GL.glDeleteProgram(prg);
        goto on_error;
     }

   r = malloc(sizeof (Ector_Shader));
   r->prg = prg;
   r->flags = flags;

   _ector_gl_shader_textures_bind(r);

 on_error:
   if (vtx) GL.glDeleteShader(vtx);
   if (frg) GL.glDeleteShader(frg);
   free(formats);

   if (!direct) free(data);
   eina_strbuf_free(buf);
   return r;
}

/**
 * @brief Retrieves or creates a shader program based on the given flags.
 *
 * This function implements a multi-level caching strategy for shader programs:
 * 1. Checks an in-memory hash table (shader_cache) for an existing Ector_Shader.
 * 2. If not found, attempts to load a pre-compiled binary from an Eet file using _ector_gl_shader_load().
 * 3. If still not found (or loading fails), compiles the shader on-the-fly using ector_gl_shader_compile().
 * 4. If a new shader is compiled and program binary support is available, the binary is saved to the Eet cache file.
 * The retrieved or created shader is added to the in-memory cache for future use.
 *
 * @param obj The Ector_GL_Surface object (unused in this function).
 * @param pd Private data of the Ector_GL_Surface object (unused in this function).
 * @param flags A bitmask of flags that uniquely identifies the desired shader program.
 *              These flags determine features like texturing, color modes, etc.
 * @return The OpenGL program ID (GLuint) of the shader. Returns (unsigned int)-1 on failure.
 */
static unsigned int
_ector_gl_surface_shader_get(Eo *obj EINA_UNUSED, Ector_GL_Surface_Data *pd EINA_UNUSED, uint64_t flags)
{
   Ector_Shader *shd;
   Eina_Strbuf *buf = NULL;
   void *data = NULL;
   int length = 0, size = 0;
   GLenum format;
   GLuint prg;

   shd = eina_hash_find(shader_cache, &flags);
   if (shd) return shd->prg;

   shd = _ector_gl_shader_load(flags);
   if (shd)
     {
        eina_hash_direct_add(shader_cache, &shd->flags, shd);
        return shd->prg;
     }

   prg = ector_gl_shader_compile(flags);
   if (prg <= 0) return -1;

   GL.glGetProgramiv(prg, GL_PROGRAM_BINARY_LENGTH, &length);
   if (length <= 0) return prg;

   if (GL.glGetProgramBinary)
     {
        data = malloc(length);
        if (!data) return prg;

        GL.glGetProgramBinary(prg, length, &size, &format, data);
        if (length != size) goto on_error;
     }

   shd = malloc(sizeof (Ector_Shader));
   if (!shd) goto on_error;

   shd->prg = prg;
   shd->flags = flags;

   // Save the shader in the cache file
   eina_hash_direct_add(shader_cache, &shd->flags, shd);

   // Save binary shader in the cache file
   if (GL.glGetProgramBinary)
     {
        buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "ector/shader/%16" PRIx64, flags);

        eet_write(shader_file, eina_strbuf_string_get(buf), data, length, 1);

        eina_strbuf_free(buf);
     }

 on_error:
   free(data);

   return prg;
}

/**
 * @brief Destructor for the Ector_GL_Surface object.
 *
 * This function is called when an Ector_GL_Surface object is being destroyed.
 * It performs cleanup operations, primarily freeing the global shader cache
 * and closing the shader cache Eet file if they were initialized.
 * It also calls the destructor of the parent class.
 *
 * @param obj The Ector_GL_Surface object being destroyed.
 * @param pd Private data of the Ector_GL_Surface object (unused in this function).
 */
static void
_ector_gl_surface_efl_object_destructor(Eo *obj, Ector_GL_Surface_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, ECTOR_GL_SURFACE_CLASS));

   eina_hash_free(shader_cache);
   shader_cache = NULL;
   eet_close(shader_file);
   shader_file = NULL;
}

/**
 * @brief Constructor for the Ector_GL_Surface object.
 *
 * This function is called when a new Ector_GL_Surface object is being created.
 * It initializes the global shader cache (if not already initialized) and attempts
 * to open or create an Eet file for storing/retrieving pre-compiled shader binaries.
 * The path for this cache file is constructed based on user's home or temporary directory
 * and EFL version. It also calls the constructor of the parent class.
 *
 * @param obj The Ector_GL_Surface object being constructed.
 * @param pd Private data of the Ector_GL_Surface object (unused in this function).
 * @return The constructed Efl_Object, or NULL on failure.
 */
static Efl_Object *
_ector_gl_surface_efl_object_constructor(Eo *obj, Ector_GL_Surface_Data *pd EINA_UNUSED)
{
   Eina_Strbuf *file_path = NULL;

   obj = efl_constructor(efl_super(obj, ECTOR_GL_SURFACE_CLASS));
   if (!obj) return NULL;

   if (shader_cache) return obj;

   // Only initialize things once
   shader_cache = eina_hash_int64_new(_shader_free);

   /* glsym_glProgramBinary = _ector_gl_symbol_get(obj, "glProgramBinary"); */
   /* glsym_glGetProgramBinary = _ector_gl_symbol_get(obj, "glGetProgramBinary"); */
   /* glsym_glProgramParameteri = _ector_gl_symbol_get(obj, "glProgramParameteri"); */
   /* glsym_glReleaseShaderCompiler = _ector_gl_symbol_get(obj, "glReleaseShaderCompiler"); */

   if (GL.glProgramBinary && GL.glGetProgramBinary)
     {
        file_path = eina_strbuf_new();
        if (eina_environment_home_get())
          eina_strbuf_append(file_path, eina_environment_home_get());
        else
          eina_strbuf_append(file_path, eina_environment_tmp_get());
        eina_strbuf_append_printf(file_path, "%c.cache", EINA_PATH_SEP_C);
        // FIXME: test and create path if necessary
        eina_strbuf_append_printf(file_path, "%cector", EINA_PATH_SEP_C);
        eina_strbuf_append_printf(file_path, "%cector-shader-%i.%i.eet",
                                  EINA_PATH_SEP_C, EFL_VERSION_MAJOR, EFL_VERSION_MINOR);
        shader_file = eet_open(eina_strbuf_string_get(file_path), EET_FILE_MODE_READ_WRITE);
     }
   if (!shader_file)
     {
        ERR("Unable to create '%s' ector binary shader file.", eina_strbuf_string_get(file_path));
        GL.glProgramBinary = NULL;
     }
   eina_strbuf_free(file_path);

   return obj;
}

#include "ector_gl_surface.eo.c"
