/**
 * @file
 * @brief Header file for the Elementary test application.
 *
 * This file defines the logging domain and macros used throughout the
 * Elementary test application.
 */
#ifndef _TEST_H
#define _TEST_H

/**
 * @brief The Eina log domain for this application.
 *
 * This variable holds the identifier for the log domain used by EINA_LOG_* macros.
 * It is initialized in test.c.
 */
extern int _log_domain;

#undef CRI
/**
 * @def CRI(...)
 * @brief Log a critical message.
 * Wrapper around EINA_LOG_DOM_CRIT.
 */
#define CRI(...)      EINA_LOG_DOM_CRIT(_log_domain, _VA_ARGS__)
#undef ERR
/**
 * @def ERR(...)
 * @brief Log an error message.
 * Wrapper around EINA_LOG_DOM_ERR.
 */
#define ERR(...)      EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#undef WRN
/**
 * @def WRN(...)
 * @brief Log a warning message.
 * Wrapper around EINA_LOG_DOM_WARN.
 */
#define WRN(...)      EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#undef INF
/**
 * @def INF(...)
 * @brief Log an informational message.
 * Wrapper around EINA_LOG_DOM_INFO.
 */
#define INF(...)      EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#undef DBG
/**
 * @def DBG(...)
 * @brief Log a debug message.
 * Wrapper around EINA_LOG_DOM_DBG.
 */
#define DBG(...)      EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)

#endif
