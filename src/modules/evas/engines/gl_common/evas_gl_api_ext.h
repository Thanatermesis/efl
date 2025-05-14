/**
 * @file
 * @brief This header defines the API for handling OpenGL and EGL extensions
 *        within Evas GL. It provides macros and functions to query for
 *        extension support and to access extension function pointers.
 */
#ifndef _EVAS_GL_API_EXT_H
#define _EVAS_GL_API_EXT_H

#include "evas_gl_core_private.h"

#ifdef GL_GLES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#else
# ifdef BUILD_ENGINE_GL_COCOA
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
# else
#  include <GL/glext.h>
#  include <GL/glx.h>
# endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Extension HEADER
/////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @name X-Macros for Extension Handling
 * @{
 * @brief A set of macros used with `evas_gl_api_ext_def.h` to automate the
 * declaration and definition of extension-related symbols. This is known as
 * the X-Macro pattern.
 *
 * In this header, these macros are defined to declare external function
 * pointers for each extension function. In `evas_gl_api_ext.c`, they are
 * redefined multiple times to perform different tasks like initializing
 * function pointers, defining wrapper functions, and checking for support.
 */
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name)
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
/**
 * @brief Begins the definition of an extension function.
 *
 * This macro declares function pointers for an extension function for each
 * relevant API (EGL, GL, GLES1, GLES3). For example, `egl_ext_sym_name`
 * will be the pointer to the EGL version of the function.
 */
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
   extern ret (*egl_ext_sym_##name) param1; \
   extern ret (*gl_ext_sym_##name) param1; \
   extern ret (*gles1_ext_sym_##name) param1; \
   extern ret (*gles3_ext_sym_##name) param1;
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)
/** @} */

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
/////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @def EXT_FUNC_EGL(fname)
 * @brief Macro to access an EGL extension function pointer.
 * @param fname The name of the EGL extension function (e.g., eglCreateImageKHR).
 * @return A function pointer to the specified EGL extension function.
 * @note This resolves to `egl_ext_sym_fname`.
 */
#define EXT_FUNC_EGL(fname) egl_ext_sym_##fname

/**
 * @def EXT_FUNC(fname)
 * @brief Macro to access a GL/GLES2 extension function pointer.
 * @param fname The name of the GL/GLES2 extension function (e.g., glDiscardFramebufferEXT).
 * @return A function pointer to the specified GL/GLES2 extension function.
 * @note This resolves to `gl_ext_sym_fname`.
 */
#define EXT_FUNC(fname) gl_ext_sym_##fname

/**
 * @def EXT_FUNC_GLES1(fname)
 * @brief Macro to access a GLES1 extension function pointer.
 * @param fname The name of the GLES1 extension function (e.g., glDrawTexiOES).
 * @return A function pointer to the specified GLES1 extension function.
 * @note This resolves to `gles1_ext_sym_fname`.
 */
#define EXT_FUNC_GLES1(fname) gles1_ext_sym_##fname

/**
 * @def EXT_FUNC_GLES3(fname)
 * @brief Macro to access a GLES3 extension function pointer.
 * @param fname The name of the GLES3 extension function (e.g., glDebugMessageControl).
 * @return A function pointer to the specified GLES3 extension function.
 * @note This resolves to `gles3_ext_sym_fname`.
 */
#define EXT_FUNC_GLES3(fname) gles3_ext_sym_##fname

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Extension HEADER
/////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @name X-Macros for Extension Support Flags
 * @{
 * @brief A redefinition of the X-Macros to declare support flag variables.
 *
 * This set of macros declares integer variables (e.g., `_gl_ext_support_name`)
 * that will be used to store whether a given extension is supported by the
 * current driver and context.
 */
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
/**
 * @brief Begins the definition of an extension.
 *
 * This macro declares integer flags to track support for an extension across
 * different APIs (EGL, GL, GLES1, GLES3). For example, `_gl_ext_support_name`
 * will be 1 if the extension `name` is supported for GL/GLES2.
 */
#define _EVASGL_EXT_BEGIN(name) \
   extern int _egl_ext_support_##name; \
   extern int _gl_ext_support_##name; \
   extern int _gles1_ext_support_##name; \
   extern int _gles3_ext_support_##name;
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)
/** @} */

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
/////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @def EXTENSION_SUPPORT_EGL(name)
 * @brief Macro to check if a specific EGL extension is supported.
 * @param name The name of the EGL extension (e.g., EGL_KHR_image_base).
 * @return 1 if the extension is supported, 0 otherwise.
 * @note This resolves to `(_egl_ext_support_name == 1)`.
 */
#define EXTENSION_SUPPORT_EGL(name) (_egl_ext_support_##name == 1)

/**
 * @def EXTENSION_SUPPORT(name)
 * @brief Macro to check if a specific GL/GLES2 extension is supported.
 * @param name The name of the GL/GLES2 extension (e.g., GL_EXT_discard_framebuffer).
 * @return 1 if the extension is supported, 0 otherwise.
 * @note This resolves to `(_gl_ext_support_name == 1)`.
 */
#define EXTENSION_SUPPORT(name) (_gl_ext_support_##name == 1)

/**
 * @def EXTENSION_SUPPORT_GLES1(name)
 * @brief Macro to check if a specific GLES1 extension is supported.
 * @param name The name of the GLES1 extension (e.g., GL_OES_draw_texture).
 * @return 1 if the extension is supported, 0 otherwise.
 * @note This resolves to `(_gles1_ext_support_name == 1)`.
 */
#define EXTENSION_SUPPORT_GLES1(name) (_gles1_ext_support_##name == 1)

/**
 * @def EXTENSION_SUPPORT_GLES3(name)
 * @brief Macro to check if a specific GLES3 extension is supported.
 * @param name The name of the GLES3 extension (e.g., GL_KHR_debug).
 * @return 1 if the extension is supported, 0 otherwise.
 * @note This resolves to `(_gles3_ext_support_name == 1)`.
 */
#define EXTENSION_SUPPORT_GLES3(name) (_gles3_ext_support_##name == 1)

#ifdef GL_GLES
/**
 * @brief Initializes EGL extensions.
 * This function queries for available EGL extensions and loads function
 * pointers for supported extensions. It should be called once during
 * Evas GL initialization.
 * @param getproc A function pointer to the EGL getProcAddress function (e.g., eglGetProcAddress).
 * @param glueexts A string containing EGL extensions provided by the glue layer,
 *                 which might not be reported by the driver directly.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
extern Eina_Bool evgl_api_egl_ext_init(void *getproc, const char *glueexts);
#endif

/**
 * @brief Populates the Evas_GL_API structure with GLES2 extension function pointers.
 * This function checks for supported GLES2 extensions and assigns the
 * corresponding function pointers in the provided Evas_GL_API structure.
 * @param gl_funcs Pointer to an Evas_GL_API structure to be populated.
 * @param getproc A function pointer to the getProcAddress function (e.g., eglGetProcAddress or glXGetProcAddress).
 * @param glueexts A string containing GL extensions provided by the glue layer.
 */
extern void evgl_api_gles2_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts);

/**
 * @brief Populates the Evas_GL_API structure with GLES1 extension function pointers.
 * Similar to evgl_api_gles2_ext_get, but for GLES1 extensions.
 * @param gl_funcs Pointer to an Evas_GL_API structure to be populated.
 * @param getproc A function pointer to the getProcAddress function.
 * @param glueexts A string containing GL extensions provided by the glue layer.
 */
extern void evgl_api_gles1_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts);

/**
 * @brief Populates the Evas_GL_API structure with GLES3 extension function pointers.
 * Similar to evgl_api_gles2_ext_get, but for GLES3 extensions.
 * @param gl_funcs Pointer to an Evas_GL_API structure to be populated.
 * @param getproc A function pointer to the getProcAddress function.
 * @param glueexts A string containing GL extensions provided by the glue layer.
 */
extern void evgl_api_gles3_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts);

/**
 * @brief Retrieves the EGL extension string.
 * This string contains a space-separated list of all supported EGL extensions.
 * @return A pointer to the EGL extension string, or NULL if EGL extensions
 *         are not initialized or not applicable. The string is owned by Evas GL
 *         and should not be freed by the caller.
 */
extern const char *evgl_api_ext_egl_string_get(void);

/**
 * @brief Retrieves the GL/GLES extension string for a specific version.
 * This string contains a space-separated list of supported GL/GLES extensions.
 * @param official If EINA_TRUE, returns only official Khronos/vendor extensions.
 *                 If EINA_FALSE, may include Evas-specific or aliased extension names.
 * @param version The GL API version (e.g., EVAS_GL_GLES_1_X, EVAS_GL_GLES_2_X, EVAS_GL_GLES_3_X).
 *                EVAS_GL_GLES_2_X is used as a fallback for desktop GL.
 * @return A pointer to the extension string, or NULL if extensions are not
 *         initialized. The string is owned by Evas GL and should not be freed.
 */
extern const char *evgl_api_ext_string_get(Eina_Bool official, int version);

/**
 * @brief Retrieves an individual GL/GLES extension string by index.
 * This function is primarily for GLES3 and later, where glGetStringi is available.
 * @param index The index of the extension string to retrieve.
 * @param version The GL API version, typically EVAS_GL_GLES_3_X.
 * @return A pointer to the extension string at the given index, or NULL if
 *         the index is out of bounds, extensions are not initialized, or the
 *         functionality is not supported for the given version. The string is
 *         owned by Evas GL.
 */
extern const char *evgl_api_ext_stringi_get(GLuint index, int version);

/**
 * @brief Retrieves the number of supported GL/GLES extensions.
 * This function is primarily for GLES3 and later, corresponding to GL_NUM_EXTENSIONS.
 * @param version The GL API version, typically EVAS_GL_GLES_3_X.
 * @return The number of supported extensions, or 0 if not applicable or
 *         extensions are not initialized.
 */
extern GLuint evgl_api_ext_num_extensions_get(int version);

#endif //_EVAS_GL_API_EXT_H

