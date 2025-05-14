/**
 * @file
 * @brief Ecore memory pool allocation functions.
 *
 * This file provides functions for managing memory pools for various Ecore
 * data structures. It aims to improve performance by reducing the overhead
 * of frequent allocations and deallocations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include <Eina.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @brief Structure to hold information about a specific Ecore mempool.
 */
typedef struct _Ecore_Mempool Ecore_Mempool;
struct _Ecore_Mempool
{
   const char *name; /**< The name of the mempool type (e.g., "Ecore_Pipe") */
   Eina_Mempool *mp; /**< Pointer to the Eina mempool instance */
   size_t size;      /**< Size of a single element in this mempool */
};

/**
 * @def GENERIC_ALLOC_FREE(TYPE, Type)
 * @brief Macro to generate mempool allocation and free functions for a given Ecore type.
 *
 * This macro defines:
 * - An external declaration for the size of the TYPE (`_ecore_sizeof_##TYPE`).
 * - An `Ecore_Mempool` static instance (`Type##_mp`) initialized with the type name.
 * - A `Type##_calloc()` function that allocates memory from the mempool.
 * - A `Type##_mp_free()` function that frees memory back to the mempool.
 *
 * @param TYPE The Ecore structure type (e.g., Ecore_Pipe).
 * @param Type The prefix for the generated functions and mempool variable (e.g., ecore_pipe).
 */
#define GENERIC_ALLOC_FREE(TYPE, Type)                                  \
  extern size_t _ecore_sizeof_##TYPE;                                   \
  Ecore_Mempool Type##_mp = { #TYPE,  NULL, 0 };                        \
  TYPE *                                                                \
  Type##_calloc(unsigned int num)                                       \
  {                                                                     \
     return eina_mempool_calloc(Type##_mp.mp,                           \
                                num * _ecore_sizeof_##TYPE);            \
  }                                                                     \
  void                                                                  \
  Type##_mp_free(TYPE *e)                                               \
  {                                                                     \
     eina_mempool_free(Type##_mp.mp, e);                                \
  }

//GENERIC_ALLOC_FREE(Ecore_Animator, ecore_animator);
//GENERIC_ALLOC_FREE(Ecore_Event_Handler, ecore_event_handler);
//GENERIC_ALLOC_FREE(Ecore_Event_Filter, ecore_event_filter);
//GENERIC_ALLOC_FREE(Ecore_Event, ecore_event);
//GENERIC_ALLOC_FREE(Ecore_Idle_Exiter, ecore_idle_exiter);
//GENERIC_ALLOC_FREE(Ecore_Idle_Enterer, ecore_idle_enterer);
//GENERIC_ALLOC_FREE(Ecore_Idler, ecore_idler);
//GENERIC_ALLOC_FREE(Ecore_Job, ecore_job);
//GENERIC_ALLOC_FREE(Ecore_Timer, ecore_timer);
//GENERIC_ALLOC_FREE(Ecore_Poller, ecore_poller);
GENERIC_ALLOC_FREE(Ecore_Pipe, ecore_pipe);
GENERIC_ALLOC_FREE(Ecore_Fd_Handler, ecore_fd_handler);
GENERIC_ALLOC_FREE(Efl_Loop_Promise_Simple_Data, efl_loop_promise_simple_data);
#ifdef _WIN32
GENERIC_ALLOC_FREE(Ecore_Win32_Handler, ecore_win32_handler);
#endif

/**
 * @brief Array of pointers to all managed Ecore_Mempool instances.
 *
 * This array is used to iterate over all mempools during initialization
 * and shutdown.
 * Example:
 * @code
 * static Ecore_Mempool *mempool_array[] = {
 *   &ecore_pipe_mp, // Mempool for Ecore_Pipe structures
 *   &ecore_fd_handler_mp, // Mempool for Ecore_Fd_Handler structures
 *   // ... other mempools
 * };
 * @endcode
 */
static Ecore_Mempool *mempool_array[] = {
//  &ecore_animator_mp,
//  &ecore_event_handler_mp,
//  &ecore_event_filter_mp,
//  &ecore_event_mp,
//  &ecore_idle_exiter_mp,
//  &ecore_idle_enterer_mp,
//  &ecore_idler_mp,
//  &ecore_job_mp,
//  &ecore_timer_mp,
//  &ecore_poller_mp,
  &ecore_pipe_mp,
  &ecore_fd_handler_mp,
  &efl_loop_promise_simple_data_mp,
#ifdef _WIN32
  &ecore_win32_handler_mp
#endif
};

/**
 * @brief Initializes all registered Ecore mempools.
 *
 * This function iterates through the `mempool_array` and initializes
 * each mempool using `eina_mempool_add()`. The type of mempool (e.g.,
 * "chained_mempool", "pass_through") can be influenced by the
 * `EINA_MEMPOOL` environment variable.
 *
 * It also sets the `size` member of each `Ecore_Mempool` structure
 * based on the `_ecore_sizeof_##TYPE` variables.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure to initialize
 *         any mempool.
 */
Eina_Bool
ecore_mempool_init(void)
{
   const char *choice;
   unsigned int i;

#define MP_SIZE_INIT(TYPE, Type) \
   Type##_mp.size = _ecore_sizeof_##TYPE

//   MP_SIZE_INIT(Ecore_Animator, ecore_animator);
//   MP_SIZE_INIT(Ecore_Event_Handler, ecore_event_handler);
//   MP_SIZE_INIT(Ecore_Event_Filter, ecore_event_filter);
//   MP_SIZE_INIT(Ecore_Event, ecore_event);
//   MP_SIZE_INIT(Ecore_Idle_Exiter, ecore_idle_exiter);
//   MP_SIZE_INIT(Ecore_Idle_Enterer, ecore_idle_enterer);
//   MP_SIZE_INIT(Ecore_Idler, ecore_idler);
//   MP_SIZE_INIT(Ecore_Job, ecore_job);
//   MP_SIZE_INIT(Ecore_Timer, ecore_timer);
//   MP_SIZE_INIT(Ecore_Poller, ecore_poller);
   MP_SIZE_INIT(Ecore_Pipe, ecore_pipe);
   MP_SIZE_INIT(Ecore_Fd_Handler, ecore_fd_handler);
   MP_SIZE_INIT(Efl_Loop_Promise_Simple_Data, efl_loop_promise_simple_data);
#ifdef _WIN32
   MP_SIZE_INIT(Ecore_Win32_Handler, ecore_win32_handler);
#endif
#undef MP_SIZE_INIT

   choice = getenv("EINA_MEMPOOL");
   if ((!choice) || (!choice[0]))
     choice = "chained_mempool";

   for (i = 0; i < sizeof (mempool_array) / sizeof (mempool_array[0]); ++i)
     {
     retry:
        mempool_array[i]->mp = eina_mempool_add(choice, mempool_array[i]->name, NULL, mempool_array[i]->size, 16);
        if (!mempool_array[i]->mp)
          {
             if (!(!strcmp(choice, "pass_through")))
               {
                  ERR("Falling back to pass through ! Previously tried '%s' mempool.", choice);
                  choice = "pass_through";
                  goto retry;
               }
             else
               {
                  ERR("Impossible to allocate mempool '%s' !", choice);
                  return EINA_FALSE;
               }
          }
     }
   return EINA_TRUE;
}

/**
 * @brief Shuts down all registered Ecore mempools.
 *
 * This function iterates through the `mempool_array` and deinitializes
 * each mempool using `eina_mempool_del()`, freeing associated resources.
 */
void
ecore_mempool_shutdown(void)
{
   unsigned int i;

   for (i = 0; i < sizeof (mempool_array) / sizeof (mempool_array[0]); ++i)
     {
        eina_mempool_del(mempool_array[i]->mp);
        mempool_array[i]->mp = NULL;
     }
}

