/**
 * @file
 * @brief This file implements the Ector GL Buffer, which provides a
 *        hardware-accelerated buffer using OpenGL.
 */

#ifdef HAVE_CONFIG_H
# include "config.h" /**< Standard config header. */
#endif

#include <Ector.h> /**< Main Ector public header. */
#include "Ector_GL.h" /**< Ector OpenGL specific public header. */
#include "ector_private.h" /**< Ector internal (private) definitions. */
#include "ector_gl_private.h" /**< Ector OpenGL internal (private) definitions. */
#include "ector_buffer.h" /**< Ector generic buffer interface. */

/**
 * @def MY_CLASS
 * @brief Macro defining the Evas Object (EO) class for this implementation file.
 *        This is a common pattern in EFL to associate the C implementation
 *        with its corresponding EO class.
 */
#define MY_CLASS ECTOR_GL_BUFFER_CLASS

#include "ector_gl_buffer.eo.c" /**< Includes the C code generated from the Ector_GL_Buffer EO definition. */
