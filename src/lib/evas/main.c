#include "evas_common_private.h"
#include "evas_private.h"

/** @internal
 * @brief Evas library version information.
 *
 * Initialized with major, minor, micro, and revision numbers.
 */
static Evas_Version _version = { VMAJ, VMIN, VMIC, VREV };

/**
 * @brief Publicly accessible Evas library version information.
 *
 * Points to the internal _version struct.
 */
EVAS_API Evas_Version *evas_version = &_version;

/** @internal
 * @brief Global flag indicating the last allocation error type in Evas.
 *
 * See #Evas_Alloc_Error for possible values.
 * 0 (EVAS_ALLOC_ERROR_NONE) means no error.
 */
int _evas_alloc_error = 0;
static int _evas_debug_init = 0;
/** @internal
 * @brief Enumeration to control the visibility of Evas debug messages.
 */
static enum {
     _EVAS_DEBUG_DEFAULT, /**< Default behavior, usually shows critical errors. */
     _EVAS_DEBUG_HIDE,    /**< Hides all Evas debug messages. */
     _EVAS_DEBUG_SHOW     /**< Shows all Evas debug messages. */
} _evas_debug_show = _EVAS_DEBUG_DEFAULT; /**< @internal @brief Current debug message visibility state. */
static int _evas_debug_abort = 0; /**< @internal @brief If set to 1, Evas will call abort() on certain debug errors. */

/**
 * @brief Retrieves the last allocation error that occurred in Evas.
 *
 * This function returns the current value of the global #_evas_alloc_error flag.
 * It can be used to check if an Evas operation failed due to memory allocation
 * issues.
 *
 * @return The last allocation error code. See #Evas_Alloc_Error for details.
 *         Example: EVAS_ALLOC_ERROR_NONE if no error,
 *                  EVAS_ALLOC_ERROR_FATAL if a critical allocation failed.
 */
EVAS_API Evas_Alloc_Error
evas_alloc_error(void)
{
   return _evas_alloc_error;
}

/**
 * @internal
 * @brief Initializes Evas debug settings from environment variables.
 *
 * This function is called internally to configure debug message visibility
 * and abort behavior based on `EVAS_DEBUG_SHOW` and `EVAS_DEBUG_ABORT`
 * environment variables.
 * - `EVAS_DEBUG_SHOW`: If set to "0", hides debug messages. If set to any
 *   other non-empty value, shows debug messages.
 * - `EVAS_DEBUG_ABORT`: If set (to any value), Evas will call `abort()`
 *   on certain critical errors.
 */
static void
_evas_debug_init_from_env(void)
{
   const char *tmp = getenv("EVAS_DEBUG_SHOW");
   if (tmp)
     {
        int dbgshow = atoi(tmp);
        _evas_debug_show = (dbgshow) ? _EVAS_DEBUG_SHOW : _EVAS_DEBUG_HIDE;
     }
   if (getenv("EVAS_DEBUG_ABORT")) _evas_debug_abort = 1;
   _evas_debug_init = 1;
}

/**
 * @internal
 * @brief Handles and reports a generic Evas error, typically a magic check failure.
 *
 * If debug messages are enabled (via `EVAS_DEBUG_SHOW` environment variable
 * or if it's the default behavior), this function prints an error message
 * "Evas Magic Check Failed!!!".
 * It does not abort.
 */
void
evas_debug_error(void)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if (_evas_debug_show == _EVAS_DEBUG_SHOW)
     CRI("Evas Magic Check Failed!!!");
}

/**
 * @internal
 * @brief Handles and reports an error where an input object pointer is NULL.
 *
 * If debug messages are enabled (via `EVAS_DEBUG_SHOW` environment variable),
 * this function prints an error message "Input object pointer is NULL!".
 * If `EVAS_DEBUG_ABORT` is set, this function will call `abort()`.
 */
void
evas_debug_input_null(void)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if (_evas_debug_show == _EVAS_DEBUG_SHOW)
     CRI("Input object pointer is NULL!");
   if (_evas_debug_abort) abort();
}

/**
 * @internal
 * @brief Prints a generic debug message.
 *
 * This function is used to output arbitrary debug strings. The message is
 * printed if debug messages are set to be shown explicitly (`_EVAS_DEBUG_SHOW`)
 * or if they are at their default setting (`_EVAS_DEBUG_DEFAULT`).
 * If `EVAS_DEBUG_ABORT` is set, this function will call `abort()`.
 *
 * @param str The debug message string to print.
 *            Example: "Custom debug information: operation failed."
 */
void
evas_debug_generic(const char *str)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if ((_evas_debug_show == _EVAS_DEBUG_SHOW) ||
         (_evas_debug_show == _EVAS_DEBUG_DEFAULT))
     CRI("%s", str);
   if (_evas_debug_abort) abort();
}
