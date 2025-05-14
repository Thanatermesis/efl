#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

/**
 * @internal
 * @brief Internal structure holding the library version components.
 */
static Embryo_Version _version = { VMAJ, VMIN, VMIC, VREV };

/**
 * @brief Pointer to the Embryo library version information.
 *
 * This variable provides access to the major, minor, micro, and revision
 * numbers of the Embryo library.
 * @since 1.0
 */
EAPI Embryo_Version * embryo_version = &_version;

/**
 * @internal
 * @brief Counter for Embryo library initialization calls.
 *
 * This counter ensures that Embryo is initialized only once and shut down
 * properly when multiple init/shutdown pairs are called.
 */
static int _embryo_init_count = 0;

/**
 * @internal
 * @brief Default Eina log domain for Embryo.
 *
 * Stores the ID of the log domain registered with Eina for Embryo messages.
 * It is -1 if not registered.
 */
int _embryo_default_log_dom = -1;

/*** EXPORTED CALLS ***/

/**
 * @brief Initializes the Embryo library.
 *
 * This function initializes all the necessary subsystems for Embryo to operate,
 * primarily by initializing the Eina library and setting up a default log domain.
 * It uses a counter to handle multiple initialization calls, ensuring that
 * actual initialization occurs only on the first call.
 *
 * @return The current initialization count. Returns 0 or a negative value on failure.
 *         A positive value indicates success, with 1 being the first successful initialization.
 * @see embryo_shutdown()
 * @since 1.0
 */
EAPI int
embryo_init(void)
{
   if (++_embryo_init_count != 1)
     return _embryo_init_count;

   if (!eina_init())
     return --_embryo_init_count;

   _embryo_default_log_dom = eina_log_domain_register
       ("embryo", EMBRYO_DEFAULT_LOG_COLOR);
   if (_embryo_default_log_dom < 0)
     {
        EINA_LOG_ERR("Embryo Can not create a general log domain.");
        goto shutdown_eina;
     }

   eina_log_timing(_embryo_default_log_dom,
                   EINA_LOG_STATE_STOP,
                   EINA_LOG_STATE_INIT);

   return _embryo_init_count;

shutdown_eina:
   eina_shutdown();
   return --_embryo_init_count;
}

/**
 * @brief Shuts down the Embryo library.
 *
 * This function shuts down the Embryo library, releasing any resources it acquired.
 * It unregisters the log domain and shuts down the Eina library.
 * It uses a counter to ensure that the library is only fully shut down when
 * the number of shutdown calls matches the number of initialization calls.
 *
 * @return The current initialization count after decrementing. Returns 0 when
 *         the library is fully shut down.
 * @see embryo_init()
 * @since 1.0
 */
EAPI int
embryo_shutdown(void)
{
   if (_embryo_init_count <= 0)
     {
        printf("%s:%i Init count not greater than 0 in shutdown.", __func__, __LINE__);
        return 0;
     }
   if (--_embryo_init_count != 0)
     return _embryo_init_count;

   eina_log_timing(_embryo_default_log_dom,
                   EINA_LOG_STATE_START,
                   EINA_LOG_STATE_SHUTDOWN);

   eina_log_domain_unregister(_embryo_default_log_dom);
   _embryo_default_log_dom = -1;
   eina_shutdown();

   return _embryo_init_count;
}

