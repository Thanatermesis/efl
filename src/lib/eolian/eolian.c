#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eo_parser.h"
#include "eolian_database.h"

/**
 * @internal
 * @brief Counter for eolian_init() calls.
 *
 * This counter tracks the number of times eolian_init() has been called.
 * It ensures that initialization and shutdown procedures are executed only
 * when the counter transitions from 0 to 1 and from 1 to 0, respectively.
 */
static int _eolian_init_counter = 0;
/**
 * @brief The Eolian log domain.
 *
 * Registered with Eina to handle logging for the Eolian library.
 */
int _eolian_log_dom = -1;
/**
 * @internal
 * @brief The Eolian prefix for path lookups.
 *
 * Used by Eina to locate data files and other resources associated with Eolian.
 */
Eina_Prefix *_eolian_prefix = NULL;

/**
 * @brief Initializes the Eolian library.
 *
 * This function sets up the necessary components for Eolian to operate,
 * including Eina, logging, and the lexer. It uses a counter to handle
 * multiple initialization calls, ensuring that the actual initialization
 * happens only once.
 *
 * @return The current initialization counter. It returns a value greater than 0
 *         on success, and EINA_FALSE (which is 0) on failure.
 *
 * @see eolian_shutdown()
 */
EOLIAN_API int eolian_init(void)
{
   const char *log_dom = "eolian";
   if (_eolian_init_counter > 0) return ++_eolian_init_counter;

   eina_init();
   _eolian_log_dom = eina_log_domain_register(log_dom, EINA_COLOR_LIGHTBLUE);
   if (_eolian_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: %s", log_dom);
        return EINA_FALSE;
     }

   eina_log_timing(_eolian_log_dom,
                   EINA_LOG_STATE_STOP,
                   EINA_LOG_STATE_INIT);

   INF("Init");

   _eolian_prefix = eina_prefix_new(NULL, eolian_init, "EOLIAN", "eolian",
                                    NULL, "", "", PACKAGE_DATA_DIR, "");
   if (!_eolian_prefix)
     {
        ERR("Could not initialize the Eolian prefix.");
        return EINA_FALSE;
     }

   eo_lexer_init();
   return ++_eolian_init_counter;
}

/**
 * @brief Shuts down the Eolian library.
 *
 * This function deinitializes Eolian, releasing resources that were
 * allocated by eolian_init(). It uses a counter to ensure that the
 * actual shutdown process occurs only when the last user has requested it.
 *
 * @return The current initialization counter. It returns 0 when the library
 *         is fully shut down.
 *
 * @see eolian_init()
 */
EOLIAN_API int eolian_shutdown(void)
{
   if (_eolian_init_counter <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }
   _eolian_init_counter--;

   if (_eolian_init_counter == 0)
     {
        INF("Shutdown");
        eina_log_timing(_eolian_log_dom,
              EINA_LOG_STATE_START,
              EINA_LOG_STATE_SHUTDOWN);

        eo_lexer_shutdown();
        eina_prefix_free(_eolian_prefix);
        _eolian_prefix = NULL;

        eina_log_domain_unregister(_eolian_log_dom);
        _eolian_log_dom = -1;
        eina_shutdown();
     }

   return _eolian_init_counter;
}

/**
 * @brief Gets the Eolian file format version.
 *
 * This function returns the version of the Eolian binary file format that this
 * version of the library supports. This can be used to check for
 * compatibility when reading .eot files.
 *
 * @return The Eolian file format version.
 */
EOLIAN_API unsigned short eolian_file_format_version_get(void)
{
   return EOLIAN_FILE_FORMAT_VERSION;
}
