#ifndef __EFREETD_H
#define __EFREETD_H

#ifdef EFREETD_DEFAULT_LOG_COLOR
#undef EFREETD_DEFAULT_LOG_COLOR
#endif
#define EFREETD_DEFAULT_LOG_COLOR "\033[36m"

extern int efreetd_log_dom; /**< Extern declaration for the efreetd log domain. */

/**
 * @name Logging Macros
 * @{
 */

#ifdef CRI
#undef CRI
#endif
/** @brief Log a critical message. */
#define CRI(...) EINA_LOG_DOM_CRIT(efreetd_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
/** @brief Log an error message. */
#define ERR(...) EINA_LOG_DOM_ERR(efreetd_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
/** @brief Log a debug message. */
#define DBG(...) EINA_LOG_DOM_DBG(efreetd_log_dom, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
/** @brief Log an informational message. */
#define INF(...) EINA_LOG_DOM_INFO(efreetd_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
/** @brief Log a warning message. */
#define WRN(...) EINA_LOG_DOM_WARN(efreetd_log_dom, __VA_ARGS__)

/** @} */

/**
 * @brief Signals the main loop to terminate.
 *
 * This function is typically called to gracefully shut down the daemon.
 */
void quit(void);

#endif
