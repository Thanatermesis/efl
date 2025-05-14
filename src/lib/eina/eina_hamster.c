/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric Bail
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

#include <stdio.h>
#include <string.h>

#include "eina_config.h"
#include "eina_types.h"
#include "eina_hamster.h"

/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @internal
 * @brief Internal variable storing a numerical representation of the Eina version.
 *
 * This variable encodes the major (VMAJ), minor (VMIN), micro (VMIC), and
 * revision (VREV) numbers into a single integer. This can be useful for
 * quick numerical comparisons of versions, though it's primarily an
 * internal mechanism. For example, version 1.2.3 rev 4 would be
 * (1 * 100 * 100 * 100) + (2 * 100 * 100) + (3 * 100) + 4 = 1020304.
 */
static const int _eina_hamster =
  (VMAJ * 100 * 100 * 100) + /* Major version component */
  (VMIN * 100 * 100      ) + /* Minor version component */
  (VMIC * 100            ) + /* Micro version component */
  (VREV                  );  /* Revision component */

/**
 * @endcond
 */

/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/*============================================================================*
*                                   API                                      *
*============================================================================*/

/**
 * @brief Gets the hamster count.
 * @return The number of available hamsters.
 *
 * This function returns how many hamsters you have. Internally, this
 * value is derived from the Eina library version numbers.
 * @see eina_version
 */
EINA_API int
eina_hamster_count(void)
{
   return _eina_hamster;
}

/**
 * @}
 */
