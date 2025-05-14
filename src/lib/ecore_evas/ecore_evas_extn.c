#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Ecore.h>
#include "Ecore_Evas.h"

/**
 * @brief Event type for a new client connection to an Ecore_Evas extension.
 * @since 1.2
 */
EAPI int ECORE_EVAS_EXTN_CLIENT_ADD = 0;
/**
 * @brief Event type for a client disconnection from an Ecore_Evas extension.
 * @since 1.2
 */
EAPI int ECORE_EVAS_EXTN_CLIENT_DEL = 0;

/**
 * @internal
 * @brief Initializes the Ecore_Evas extension module.
 *
 * This function registers new event types for client connections and
 * disconnections. It should be called once at startup.
 */
void
_ecore_evas_extn_init(void)
{
   ECORE_EVAS_EXTN_CLIENT_ADD = ecore_event_type_new();
   ECORE_EVAS_EXTN_CLIENT_DEL = ecore_event_type_new();
}

/**
 * @internal
 * @brief Shuts down the Ecore_Evas extension module.
 *
 * This function flushes any pending client add/delete events.
 * It should be called once at shutdown.
 */
void
_ecore_evas_extn_shutdown(void)
{
   ecore_event_type_flush(ECORE_EVAS_EXTN_CLIENT_ADD,
                          ECORE_EVAS_EXTN_CLIENT_DEL);
}
