/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric BAIL
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

/**
 * @file
 * @brief This file implements a pass-through memory pool backend for Eina.
 *
 * This backend directly uses the system's malloc, free, and realloc
 * functions without any additional pooling or tracking, serving as a
 * basic or fallback memory management strategy.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include "eina_types.h"
#include "eina_module.h"
#include "eina_mempool.h"
#include "eina_private.h"

/**
 * @brief Allocates memory using the system's malloc.
 * @param data Unused context data for the mempool backend.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
static void *
eina_pass_through_malloc(EINA_UNUSED void *data, unsigned int size)
{
   return malloc(size);
}

/**
 * @brief Frees memory using the system's free.
 * @param data Unused context data for the mempool backend.
 * @param ptr Pointer to the memory to be freed.
 */
static void
eina_pass_through_free(EINA_UNUSED void *data, void *ptr)
{
   free(ptr);
}

/**
 * @brief Checks if a pointer belongs to this memory pool.
 * @param data Unused context data for the mempool backend.
 * @param ptr The pointer to check.
 * @return Always returns EINA_TRUE, as this backend doesn't track allocations.
 * @note This function provides a best-effort guess and might not be accurate.
 */
static Eina_Bool
eina_pass_through_from(EINA_UNUSED void *data, void *ptr EINA_UNUSED)
{
   // Good luck
   return EINA_TRUE;
}

/**
 * @brief Reallocates memory using the system's realloc.
 * @param data Unused context data for the mempool backend.
 * @param ptr Pointer to the memory block to be reallocated.
 * @param size The new size for the memory block.
 * @return A pointer to the reallocated memory, or NULL on failure.
 */
static void *
eina_pass_through_realloc(EINA_UNUSED void *data, void *ptr, unsigned int size)
{
   return realloc(ptr, size);
}

/**
 * @brief Initializes the pass-through memory pool backend.
 * @param context Unused context string.
 * @param option Unused option string.
 * @param args Unused variable argument list.
 * @return A non-NULL dummy pointer indicating success, as no real initialization is needed.
 */
static void *
eina_pass_through_init(EINA_UNUSED const char *context,
                       EINA_UNUSED const char *option,
                       EINA_UNUSED va_list args)
{
   return (void *)0x1;
}

/**
 * @brief Shuts down the pass-through memory pool backend.
 * @param data Unused context data (the dummy pointer from init).
 * @note This function does nothing as there are no resources to release.
 */
static void
eina_pass_through_shutdown(EINA_UNUSED void *data)
{
}


static Eina_Mempool_Backend _eina_pass_through_mp_backend = {
   "pass_through",
   &eina_pass_through_init,
   &eina_pass_through_free,
   &eina_pass_through_malloc,
   &eina_pass_through_realloc,
   NULL,
   NULL,
   &eina_pass_through_shutdown,
   NULL,
   &eina_pass_through_from,
   NULL,
   NULL
};

/**
 * @brief Registers the pass-through memory pool backend with Eina.
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise.
 * @ingroup Eina_Mempool_Backend
 */
Eina_Bool pass_through_init(void)
{
   return eina_mempool_register(&_eina_pass_through_mp_backend);
}

/**
 * @brief Unregisters the pass-through memory pool backend from Eina.
 * @ingroup Eina_Mempool_Backend
 */
void pass_through_shutdown(void)
{
   eina_mempool_unregister(&_eina_pass_through_mp_backend);
}

#ifndef EINA_STATIC_BUILD_PASS_THROUGH

EINA_MODULE_INIT(pass_through_init);
EINA_MODULE_SHUTDOWN(pass_through_shutdown);

#endif /* ! EINA_STATIC_BUILD_PASS_THROUGH */
