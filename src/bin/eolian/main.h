/**
 * @file
 * @brief Main header file for the Eolian C generator.
 * This file defines global variables, logging macros, and utility function
 * prototypes used throughout the Eolian C generator.
 */
#ifndef EOLIAN_GEN_MAIN_H
#define EOLIAN_GEN_MAIN_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Eolian.h>

/**
 * @brief Log domain for the Eolian generator.
 * Used by EINA_LOG_DOM_* macros for logging messages.
 */
extern int _eolian_gen_log_dom;

#ifdef ERR
# undef ERR
#endif
/** @brief Macro for logging error messages. */
#define ERR(...) EINA_LOG_DOM_ERR(_eolian_gen_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
/** @brief Macro for logging debug messages. */
#define DBG(...) EINA_LOG_DOM_DBG(_eolian_gen_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
/** @brief Macro for logging informational messages. */
#define INF(...) EINA_LOG_DOM_INFO(_eolian_gen_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
/** @brief Macro for logging warning messages. */
#define WRN(...) EINA_LOG_DOM_WARN(_eolian_gen_log_dom, __VA_ARGS__)

#ifdef CRIT
# undef CRIT
#endif
/** @brief Macro for logging critical messages. */
#define CRIT(...) EINA_LOG_DOM_CRIT(_eolian_gen_log_dom, __VA_ARGS__)

/**
 * @brief Converts an Eolian name (e.g., "My.Object.Name") to a C-style full name
 *        (e.g., "my_object_name").
 * @param nm The Eolian name string.
 * @return A newly allocated string with the C-style name, or NULL on failure.
 *         The caller is responsible for freeing the returned string.
 */
char *eo_gen_c_full_name_get(const char *nm);
/**
 * @brief Generates C-style names for a given Eolian class.
 *
 * This function produces three versions of the class name:
 * - The standard C name (e.g., "my_class_name").
 * - An uppercase version (e.g., "MY_CLASS_NAME").
 * - A lowercase version (e.g., "my_class_name").
 *
 * @param cl The Eolian class.
 * @param[out] cname Pointer to store the standard C name. If NULL, this name is not generated.
 *                   The caller is responsible for freeing the allocated string if `cname` is not NULL.
 * @param[out] cnameu Pointer to store the uppercase C name. If NULL, this name is not generated.
 *                    The caller is responsible for freeing the allocated string if `cnameu` is not NULL.
 * @param[out] cnamel Pointer to store the lowercase C name. If NULL, this name is not generated.
 *                    The caller is responsible for freeing the allocated string if `cnamel` is not NULL.
 * @note If any allocation fails, the function will abort.
 */
void eo_gen_class_names_get(const Eolian_Class *cl, char **cname,
                            char **cnameu, char **cnamel);

#endif
