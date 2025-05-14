/* ECTOR - EFL retained mode drawing library
 * Copyright (C) 2014 Cedric Bail
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ector.h>
#include "ector_private.h"

/**
 * @internal
 * @brief Global structure to hold OpenGL function pointers.
 *
 * This structure is populated by ector_glsym_set() and provides
 * access to the required OpenGL functions throughout the Ector library.
 * The 'init' member indicates if the GL symbols have been successfully loaded.
 */
Ector_GL_API GL;

/**
 * @internal
 * @brief Global log domain for Ector.
 *
 * This variable holds the Eina log domain identifier registered for Ector.
 * It is used for logging messages specific to the Ector library.
 */
int _ector_log_dom_global = 0;

/**
 * @internal
 * @brief Initialization counter for the Ector library.
 *
 * This counter tracks the number of times ector_init() has been called
 * minus the number of times ector_shutdown() has been called. The library
 * is actually initialized on the first call to ector_init() and shut down
 * when the counter reaches zero after a call to ector_shutdown().
 */
static int _ector_main_count = 0;

/**
 * @brief Initializes the Ector library.
 *
 * This function initializes all the Ector subsystems. It must be called
 * before any other Ector function. It internally calls eina_init() and
 * efl_object_init().
 *
 * It also registers a log domain for Ector.
 *
 * This function maintains an internal counter. For each call to ector_init(),
 * a corresponding call to ector_shutdown() must be made. The actual
 * initialization happens on the first call, and the actual shutdown happens
 * when the counter reaches zero.
 *
 * @return The new initialization counter value. Returns 0 on failure.
 * @see ector_shutdown()
 */
ECTOR_API int
ector_init(void)
{
   if (EINA_LIKELY(_ector_main_count > 0))
     return ++_ector_main_count;

   eina_init();
   efl_object_init();

   _ector_log_dom_global = eina_log_domain_register("ector", ECTOR_DEFAULT_LOG_COLOR);
   if (_ector_log_dom_global < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ector");
        goto on_error;
     }

   _ector_main_count = 1;

   eina_log_timing(_ector_log_dom_global, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);
   GL.init = 0;

   return _ector_main_count;

 on_error:
   efl_object_shutdown();
   eina_shutdown();

   return 0;
}

/**
 * @internal
 * @brief A no-operation function.
 *
 * This function does nothing. It is used as a placeholder for OpenGL
 * function pointers if a specific function cannot be loaded via glsym.
 * This prevents null pointer dereferences when attempting to call an
 * unloaded GL function.
 */
static void
donothing(void)
{
}

/**
 * @brief Sets the OpenGL function symbol resolver.
 *
 * This function is used to provide Ector with the necessary OpenGL
 * functions. It takes a function pointer `glsym` which Ector will use
 * to look up OpenGL function symbols (e.g., `glActiveTexture`, `glBindBuffer`).
 *
 * If a symbol cannot be found, it will be replaced by a `donothing` function
 * to prevent crashes, and the function will return @c EINA_FALSE.
 *
 * @param glsym A function pointer that takes a library handle and a symbol name,
 *              and returns a pointer to the symbol.
 *              Example: `dlsym` on POSIX systems.
 * @param lib The library handle to be passed to `glsym` when looking up symbols.
 *            Example: `RTLD_DEFAULT` or a handle from `dlopen`.
 * @return @c EINA_TRUE if all essential symbols were successfully loaded (or at least
 *         assigned a `donothing` stub), @c EINA_FALSE if `glsym` is NULL or if
 *         critical symbol loading failed (though current implementation always tries
 *         to stub and returns based on `glsym` presence).
 *         The `GL.init` flag is set to the return value.
 */
ECTOR_API Eina_Bool
ector_glsym_set(void *(*glsym)(void *lib, const char *name), void *lib)
{
   Eina_Bool r = EINA_TRUE;

   if (!glsym) return EINA_FALSE;

#define ORD(a) do { GL.a = glsym(lib, #a); if (!GL.a) { GL.a = (void*) donothing; r = EINA_FALSE; } } while (0)

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
   ORD(glGetError);
   ORD(glGetFloatv);
   ORD(glGetFramebufferAttachmentParameteriv);
   ORD(glGetIntegerv);
   ORD(glGetProgramiv);
   ORD(glGetProgramInfoLog);
   ORD(glGetProgramBinary);
   ORD(glGetRenderbufferParameteriv);
   ORD(glGetShaderiv);
   ORD(glGetShaderInfoLog);
   ORD(glGetShaderPrecisionFormat);
   ORD(glGetShaderSource);
   ORD(glGetString);
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
   ORD(glProgramBinary);
   ORD(glProgramParameteri);
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

   GL.init = r;
   return r;
}

/**
 * @brief Shuts down the Ector library.
 *
 * This function shuts down all the Ector subsystems. It must be called
 * when Ector is no longer needed. It internally calls efl_object_shutdown()
 * and eina_shutdown() when the internal initialization counter reaches zero.
 *
 * For each call to ector_init(), a corresponding call to ector_shutdown()
 * must be made. The library is only truly shut down when the counter
 * (decremented by this call) reaches zero.
 *
 * @return The new initialization counter value. Returns 0 if the library was
 *         fully shut down or if there was an error (e.g., shutting down
 *         without being initialized).
 * @see ector_init()
 */
ECTOR_API int
ector_shutdown(void)
{
   if (_ector_main_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown of ector.");
        return 0;
     }

   _ector_main_count--;
   if (EINA_LIKELY(_ector_main_count > 0))
     return _ector_main_count;

   GL.init = 0;
   eina_log_timing(_ector_log_dom_global,
                   EINA_LOG_STATE_START,
                   EINA_LOG_STATE_SHUTDOWN);

   efl_object_shutdown();

   eina_log_domain_unregister(_ector_log_dom_global);

   eina_shutdown();
   return _ector_main_count;
}
