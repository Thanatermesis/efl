/**
 * @file
 * @brief This file implements the Efl.Net.Socket class, providing network socket functionalities.
 * It includes necessary Ecore and Ecore_Con headers and defines protected access macros
 * for related Eo classes.
 */

#define EFL_IO_READER_PROTECTED 1 /**< Enables access to protected members of Efl.Io.Reader */
#define EFL_IO_WRITER_PROTECTED 1 /**< Enables access to protected members of Efl.Io.Writer */
#define EFL_IO_CLOSER_PROTECTED 1 /**< Enables access to protected members of Efl.Io.Closer */
#define EFL_NET_SOCKET_PROTECTED 1 /**< Enables access to protected members of Efl.Net.Socket */

#ifdef HAVE_CONFIG_H
# include <config.h> /**< Standard build system configuration header */
#endif

#include "Ecore.h" /**< Core Efl library providing main loop and utility functions */
#include "Ecore_Con.h" /**< Ecore Connection library for network communication */
#include "ecore_con_private.h" /**< Private headers for Ecore_Con, used for implementing its features */

#include "efl_net_socket.eo.c" /**< Generated C source for the Efl.Net.Socket Eo class */
