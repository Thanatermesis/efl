/**
 * @brief Initializes the prefix detection system.
 * @param argv0 The program's argv[0], used to help locate paths.
 * @return 1 on success, 0 on failure.
 * @see e_prefix_determine() in embryo_cc_prefix.c for more details.
 */
int         e_prefix_determine(char *argv0);

/**
 * @brief Shuts down the prefix detection system.
 * @see e_prefix_shutdown() in embryo_cc_prefix.c for more details.
 */
void        e_prefix_shutdown(void);

/**
 * @brief Gets the detected installation prefix.
 * @return The prefix path (e.g., "/usr/local") or NULL if not initialized.
 *         The returned string must not be freed.
 */
const char *e_prefix_get(void);

/**
 * @brief Gets the detected binary directory path (e.g., PREFIX/bin).
 * @return The binary directory path or NULL if not initialized.
 *         The returned string must not be freed.
 */
const char *e_prefix_bin_get(void);

/**
 * @brief Gets the detected data directory path (e.g., PREFIX/share/embryo).
 * @return The data directory path or NULL if not initialized.
 *         The returned string must not be freed.
 */
const char *e_prefix_data_get(void);

/**
 * @brief Gets the detected library directory path (e.g., PREFIX/lib).
 * @return The library directory path or NULL if not initialized.
 *         The returned string must not be freed.
 */
const char *e_prefix_lib_get(void);
