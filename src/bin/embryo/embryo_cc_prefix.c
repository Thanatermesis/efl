#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>

#include "embryo_cc_prefix.h"

/* local subsystem functions */

/* local subsystem globals */

static Eina_Prefix *pfx = NULL;

/* externally accessible functions */

/**
 * @brief Initializes the prefix detection system.
 *
 * This function sets up the Eina_Prefix object that is used to determine
 * various installation paths for the Embryo library and related tools.
 * It should be called early in the application's lifecycle.
 *
 * @param argv0 The first command-line argument (program name), used to help
 *              locate the application and its resources.
 * @return 1 on success, 0 on failure.
 */
int
e_prefix_determine(char *argv0)
{
   if (pfx) return 1;
   eina_init();
   pfx = eina_prefix_new(argv0, e_prefix_determine,
                         "EMBRYO", "embryo", "include/default.inc",
                         PACKAGE_BIN_DIR,
                         PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR,
                         PACKAGE_DATA_DIR);
   if (!pfx) return 0;
   return 1;
}

/**
 * @brief Shuts down the prefix detection system.
 *
 * This function frees the resources allocated by e_prefix_determine()
 * and shuts down Eina if it was initialized by this module.
 */
void
e_prefix_shutdown(void)
{
   eina_prefix_free(pfx);
   pfx = NULL;
   eina_shutdown();
}

/**
 * @brief Retrieves the main installation prefix path.
 *
 * @return A pointer to a string containing the prefix path (e.g., "/usr/local").
 *         The returned string should not be modified or freed by the caller.
 *         Returns NULL if the prefix system is not initialized.
 */
const char *
e_prefix_get(void)
{
   return eina_prefix_get(pfx);
}

/**
 * @brief Retrieves the binary directory path.
 *
 * This is typically `PREFIX/bin`.
 *
 * @return A pointer to a string containing the binary directory path.
 *         The returned string should not be modified or freed by the caller.
 *         Returns NULL if the prefix system is not initialized.
 */
const char *
e_prefix_bin_get(void)
{
   return eina_prefix_bin_get(pfx);
}

/**
 * @brief Retrieves the data directory path.
 *
 * This is typically `PREFIX/share/embryo` or similar.
 *
 * @return A pointer to a string containing the data directory path.
 *         The returned string should not be modified or freed by the caller.
 *         Returns NULL if the prefix system is not initialized.
 */
const char *
e_prefix_data_get(void)
{
   return eina_prefix_data_get(pfx);
}

/**
 * @brief Retrieves the library directory path.
 *
 * This is typically `PREFIX/lib`.
 *
 * @return A pointer to a string containing the library directory path.
 *         The returned string should not be modified or freed by the caller.
 *         Returns NULL if the prefix system is not initialized.
 */
const char *
e_prefix_lib_get(void)
{
   return eina_prefix_lib_get(pfx);
}
