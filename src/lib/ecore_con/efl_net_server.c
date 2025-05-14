/**
 * @file
 * @brief This file is the C source implementation for the Efl_Net_Server EO interface.
 *
 * It includes the necessary Ecore and Ecore_Con headers,
 * as well as the C code generated from the efl_net_server.eo file.
 * The EFL_NET_SERVER_PROTECTED macro is defined to enable access to
 * protected API functions within the Efl.Net.Server implementation.
 */

#define EFL_NET_SERVER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#include "efl_net_server.eo.c"
