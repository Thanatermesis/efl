#include <Ecore_Buffer_Queue.h>
#include "ecore_buffer_private.h"
#include "ecore_buffer_con.h"

/**
 * @file
 * @brief This file implements the Ecore_Buffer_Queue library initialization and shutdown.
 *
 * Ecore_Buffer_Queue is a library for managing queues of Ecore_Buffer objects.
 * It provides a mechanism for inter-process communication and data sharing
 * through shared memory buffers.
 */

int _ecore_buffer_queue_log_dom = -1; /**< Log domain for Ecore_Buffer_Queue. */
static int _ecore_buffer_queue_init_count = 0; /**< Initialization counter for the library. Ensures init/shutdown is balanced. */

/**
 * @brief Initializes the Ecore_Buffer_Queue library.
 *
 * This function sets up the necessary resources for the Ecore_Buffer_Queue
 * library, including logging and connection handling. It uses a reference
 * counter to manage multiple initializations.
 *
 * @return The current initialization count. Returns 0 or a negative value on failure.
 * @see ecore_buffer_queue_shutdown()
 */
EAPI int
ecore_buffer_queue_init(void)
{
   if (++_ecore_buffer_queue_init_count != 1)
     return _ecore_buffer_queue_init_count;

   _ecore_buffer_queue_log_dom =
      eina_log_domain_register("ecore_buffer_queue", EINA_COLOR_GREEN);

   if (_ecore_buffer_queue_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ecore_buffer_queue");
        goto err;
     }

#ifdef DEBUG
   eina_log_abort_on_critical_level_set(EINA_LOG_LEVEL_ERR);
   eina_log_abort_on_critical_set(EINA_TRUE);
#endif

   DBG("Ecore_Buffer_Queue Init");

   if (!_ecore_buffer_con_init())
     {
        eina_log_domain_unregister(_ecore_buffer_queue_log_dom);
        _ecore_buffer_queue_log_dom = -1;
        goto err;
     }

   return _ecore_buffer_queue_init_count;
err:
   return --_ecore_buffer_queue_init_count;
}

/**
 * @brief Shuts down the Ecore_Buffer_Queue library.
 *
 * This function releases resources used by the Ecore_Buffer_Queue library.
 * It uses a reference counter to ensure that the library is only shut down
 * when the initialization count reaches zero.
 *
 * @return The current initialization count. After a successful final shutdown, this will be 0.
 * @see ecore_buffer_queue_init()
 */
EAPI int
ecore_buffer_queue_shutdown(void)
{
   if (--_ecore_buffer_queue_init_count != 0)
     return _ecore_buffer_queue_init_count;

   DBG("Ecore_Buffer_Queue Shutdown");
   _ecore_buffer_con_shutdown();
   eina_log_domain_unregister(_ecore_buffer_queue_log_dom);
   _ecore_buffer_queue_log_dom = -1;

   return _ecore_buffer_queue_init_count;
}
