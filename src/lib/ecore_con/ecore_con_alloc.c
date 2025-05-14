#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Ecore.h"
#include "ecore_private.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Structure to hold information about a memory pool for Ecore_Con objects.
 */
typedef struct _Ecore_Con_Mempool Ecore_Con_Mempool;
struct _Ecore_Con_Mempool
{
   const char   *name; /**< The name of the mempool. */
   Eina_Mempool *mp;   /**< Pointer to the Eina_Mempool instance. */
   size_t        size; /**< The size of each element in the mempool. */
};

/**
 * @brief Macro to generate allocation and free functions for a given Ecore_Con event type.
 *
 * This macro defines a static Ecore_Con_Mempool instance and two functions:
 * - `<Type>_alloc()`: Allocates an object of type `TYPE` from the mempool.
 * - `<Type>_free()`: Frees an object of type `TYPE` back to the mempool.
 *
 * @param TYPE The actual C type to be allocated (e.g., Ecore_Con_Event_Client_Add).
 * @param Type The prefix for the generated functions (e.g., ecore_con_event_client_add).
 */
#define GENERIC_ALLOC_FREE(TYPE, Type)                          \
  Ecore_Con_Mempool Type##_mp = { #TYPE, NULL, sizeof (TYPE) }; \
                                                                \
  TYPE *                                                        \
  Type##_alloc(void)                                            \
  {                                                             \
     return eina_mempool_malloc(Type##_mp.mp, sizeof (TYPE));   \
  }                                                             \
                                                                \
  void                                                          \
    Type##_free(TYPE * e)                                       \
  {                                                             \
     eina_mempool_free(Type##_mp.mp, e);                        \
  }

GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Add, ecore_con_event_client_add);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Del, ecore_con_event_client_del);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Write, ecore_con_event_client_write);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Data, ecore_con_event_client_data);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Error, ecore_con_event_server_error);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Upgrade, ecore_con_event_server_upgrade);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Error, ecore_con_event_client_error);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Client_Upgrade, ecore_con_event_client_upgrade);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Add, ecore_con_event_server_add);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Del, ecore_con_event_server_del);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Write, ecore_con_event_server_write);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Server_Data, ecore_con_event_server_data);
GENERIC_ALLOC_FREE(Ecore_Con_Event_Proxy_Bind, ecore_con_event_proxy_bind);

/**
 * @brief Array of pointers to all Ecore_Con_Mempool instances.
 *
 * This array is used to iterate over all mempools for initialization and shutdown.
 * Each element is a pointer to a statically defined Ecore_Con_Mempool,
 * for example:
 * @code
 * Ecore_Con_Mempool ecore_con_event_client_add_mp = { "Ecore_Con_Event_Client_Add", NULL, sizeof (Ecore_Con_Event_Client_Add) };
 * // ... and so on for other event types
 *
 * static Ecore_Con_Mempool *mempool_array[] = {
 *    &ecore_con_event_client_add_mp,
 *    &ecore_con_event_client_del_mp,
 *    // ...
 * };
 * @endcode
 */
static Ecore_Con_Mempool *mempool_array[] = {
   &ecore_con_event_client_add_mp,
   &ecore_con_event_client_del_mp,
   &ecore_con_event_client_write_mp,
   &ecore_con_event_client_data_mp,
   &ecore_con_event_server_error_mp,
   &ecore_con_event_server_upgrade_mp,
   &ecore_con_event_client_error_mp,
   &ecore_con_event_client_upgrade_mp,
   &ecore_con_event_server_add_mp,
   &ecore_con_event_server_del_mp,
   &ecore_con_event_server_write_mp,
   &ecore_con_event_server_data_mp,
   &ecore_con_event_proxy_bind_mp
};

/**
 * @brief Initializes all memory pools for Ecore_Con event objects.
 *
 * This function iterates through the `mempool_array` and initializes
 * each mempool using `eina_mempool_add`. It respects the `EINA_MEMPOOL`
 * environment variable to select the mempool type (e.g., "chained_mempool",
 * "pass_through"). If initialization with the chosen type fails, it attempts
 * to fall back to "pass_through".
 */
void
ecore_con_mempool_init(void)
{
   const char *choice;
   unsigned int i;

   choice = getenv("EINA_MEMPOOL");
   if (!choice || !choice[0])
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
                  return;
               }
          }
     }
}

/**
 * @brief Shuts down all memory pools used by Ecore_Con.
 *
 * This function iterates through the `mempool_array` and deinitializes
 * each mempool using `eina_mempool_del`. It also sets the mempool pointer
 * in each `Ecore_Con_Mempool` structure to `NULL`.
 */
void
ecore_con_mempool_shutdown(void)
{
   unsigned int i;

   for (i = 0; i < sizeof (mempool_array) / sizeof (mempool_array[0]); ++i)
     {
        eina_mempool_del(mempool_array[i]->mp);
        mempool_array[i]->mp = NULL;
     }
}

