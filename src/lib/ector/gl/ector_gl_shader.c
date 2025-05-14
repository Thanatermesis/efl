#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "gl/Ector_GL.h"
#include "ector_private.h"
#include "ector_gl_private.h"

#include "shader/ector_gl_shaders.x"

/**
 * @brief Array of strings representing shader flags.
 * Each string corresponds to a specific shader feature or optimization.
 * These flags are used to generate preprocessor directives (e.g., #define SHD_TEX)
 * in the shader source code, enabling or disabling specific code paths.
 */
static const char *_shader_flags[SHADER_FLAG_COUNT] = {
  "TEX",        /**< Texture mapping enabled */
  "BGRA",       /**< BGRA pixel format (instead of RGBA) */
  "MASK",       /**< Masking enabled */
  "SAM12",      /**< Sampler 1 and 2 used */
  "SAM21",      /**< Sampler 2 and 1 used (alternative ordering) */
  "SAM22",      /**< Sampler 2 used twice */
  "MASKSAM12",  /**< Mask sampler 1 and 2 used */
  "MASKSAM21",  /**< Mask sampler 2 and 1 used */
  "MASKSAM22",  /**< Mask sampler 2 used twice */
  "IMG",        /**< Image specific operations */
  "BIGENDIAN",  /**< Big-endian data format */
  "YUV",        /**< YUV color space */
  "YUY2",       /**< YUY2 pixel format */
  "NV12",       /**< NV12 pixel format */
  "YUV_709",    /**< YUV color space with BT.709 coefficients */
  "EXTERNAL",   /**< External texture (e.g., EGL_EXTERNAL_IMAGE_KHR) */
  "AFILL",      /**< Alpha fill mode */
  "NOMUL",      /**< No multiplication (direct color) */
  "ALPHA",      /**< Alpha channel processing */
  "RGB_A_PAIR", /**< RGB and Alpha are paired in texture (e.g. ETC1+Alpha) */
};

/**
 * @brief Generates the GLSL shader source code by prepending preprocessor directives based on flags.
 *
 * This function takes a set of flags and a base GLSL shader string. It iterates
 * through the known shader flags. If a flag is set in the `flags` bitmask,
 * a corresponding `#define SHD_FLAGNAME` directive is added to the beginning
 * of the shader source. Finally, the base shader code is appended.
 *
 * @param flags A bitmask representing the shader features to enable.
 *              Each bit corresponds to an entry in `_shader_flags`.
 *              Example: (1 << 0) | (1 << 2) would enable SHD_TEX and SHD_MASK.
 * @param base The base GLSL shader source code (either vertex or fragment).
 * @return A new Eina_Strbuf containing the complete GLSL shader source,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         this strbuf.
 */
static Eina_Strbuf *
_ector_gl_shader_glsl_get(uint64_t flags, const char *base)
{
   Eina_Strbuf *r;
   unsigned int k;

   r = eina_strbuf_new();
   for (k =0; k < SHADER_FLAG_COUNT; k++)
     {
        if (flags & (1 << k))
          eina_strbuf_append_printf(r, "#define SHD_%s\n", _shader_flags[k]);
     }

   eina_strbuf_append(r, base);

   return r;
}

/**
 * @brief Compiles a GLSL shader.
 *
 * This function takes a GL shader object ID, the shader source code, and a string
 * indicating the shader type (e.g., "vertex", "fragment"). It attempts to compile
 * the shader and logs any errors encountered.
 *
 * @param s The OpenGL shader object ID (created by glCreateShader).
 * @param shader An Eina_Strbuf containing the GLSL shader source code.
 * @param type A string describing the type of shader (e.g., "vertex", "fragment"),
 *             used for error reporting.
 * @return GL_TRUE if compilation was successful, GL_FALSE otherwise.
 */
static GLint
_ector_gl_shader_glsl_compile(GLuint s, const Eina_Strbuf *shader, const char *type)
{
   const char *str;
   GLint ok = 0;

   str = eina_strbuf_string_get(shader);

   GL.glShaderSource(s, 1, &str, NULL);
   GL.glCompileShader(s);
   GL.glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
   if (!ok)
     {
        Eina_Strbuf *err;

        err = eina_strbuf_new();
        if (!err) goto on_error;
        eina_strbuf_append_printf(err, "compile of %s shader", type);

        gl_compile_link_error(s, eina_strbuf_string_get(err), EINA_TRUE);
        ERR("Abort %s:\n%s", eina_strbuf_string_get(err), str);

        eina_strbuf_free(err);
     }

 on_error:
   return ok;
}

/**
 * @brief Links vertex and fragment shaders into a GL program.
 *
 * This function compiles the provided vertex and fragment shader sources,
 * then links them into a new GL program object. It also binds standard
 * attribute locations before linking.
 *
 * @param flags The bitmask of shader flags used for compiling the shaders.
 *              This is primarily used for error reporting if linking fails.
 * @param vertex An Eina_Strbuf containing the vertex shader source code.
 * @param fragment An Eina_Strbuf containing the fragment shader source code.
 * @return The OpenGL program object ID if linking was successful, or 0 on failure.
 *         Shader objects used for compilation are deleted before returning.
 */
static GLint
_ector_gl_shader_glsl_link(uint64_t flags,
                           const Eina_Strbuf *vertex,
                           const Eina_Strbuf *fragment)
{
   GLuint vtx = 0, frg = 0, prg = 0;
   GLint ok = 0;

   vtx = GL.glCreateShader(GL_VERTEX_SHADER);
   frg = GL.glCreateShader(GL_FRAGMENT_SHADER);

   // Compiling vertex shader
   ok = _ector_gl_shader_glsl_compile(vtx, vertex, "vertex");
   if (!ok) goto on_error;

   // Compile fragment shader
   ok = _ector_gl_shader_glsl_compile(frg, fragment, "fragment");
   if (!ok) goto on_error;

   // Link both shader together
   ok = 0;

   prg = GL.glCreateProgram();
#ifndef GL_GLES
   if ((GL.glGetProgramBinary) && (GL.glProgramParameteri))
     GL.glProgramParameteri(prg, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
#endif
   GL.glAttachShader(prg, vtx);
   GL.glAttachShader(prg, frg);

   GL.glBindAttribLocation(prg, SHAD_VERTEX,  "vertex");
   GL.glBindAttribLocation(prg, SHAD_COLOR,   "color");
   GL.glBindAttribLocation(prg, SHAD_TEXUV,   "tex_coord");
   GL.glBindAttribLocation(prg, SHAD_TEXUV2,  "tex_coord2");
   GL.glBindAttribLocation(prg, SHAD_TEXUV3,  "tex_coord3");
   GL.glBindAttribLocation(prg, SHAD_TEXA,    "tex_coorda");
   GL.glBindAttribLocation(prg, SHAD_TEXSAM,  "tex_sample");
   GL.glBindAttribLocation(prg, SHAD_MASK,    "mask_coord");
   GL.glBindAttribLocation(prg, SHAD_MASKSAM, "tex_masksample");

   GL.glLinkProgram(prg);
   GL.glGetProgramiv(prg, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(prg, "link fragment and vertex shaders", EINA_FALSE);
        ERR("Abort compile of shader (flags: %16" PRIx64 ")", flags);
        GL.glDeleteProgram(prg);
        prg = 0;
        goto on_error;
     }

 on_error:
   if (vtx) GL.glDeleteShader(vtx);
   if (frg) GL.glDeleteShader(frg);

   return prg;
}

/**
 * @brief Compiles and links a complete GL shader program based on flags.
 *
 * This is the main entry point for creating a shader program. It generates
 * the vertex and fragment shader sources using `_ector_gl_shader_glsl_get`,
 * then links them using `_ector_gl_shader_glsl_link`.
 *
 * @param flags A bitmask representing the shader features to enable.
 *              This determines which preprocessor directives are added to the
 *              base vertex and fragment shaders.
 *              Example: (1 << 0) | (1 << 3) would enable SHD_TEX and SHD_SAM12.
 * @return The OpenGL program object ID if compilation and linking were successful,
 *         or 0 on failure.
 */
GLuint
ector_gl_shader_compile(uint64_t flags)
{
   Eina_Strbuf *vertex, *fragment;
   GLuint shd = 0;

   vertex = _ector_gl_shader_glsl_get(flags, vertex_glsl);
   fragment = _ector_gl_shader_glsl_get(flags, fragment_glsl);
   if (!vertex || !fragment) goto on_error;

   shd = _ector_gl_shader_glsl_link(flags, vertex, fragment);

 on_error:
   eina_strbuf_free(vertex);
   eina_strbuf_free(fragment);

#ifdef GL_GLES
   GL.glReleaseShaderCompiler();
#else
   if (GL.glReleaseShaderCompiler)
     GL.glReleaseShaderCompiler();
#endif

   return shd;
}
