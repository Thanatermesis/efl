#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <unistd.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @internal
 * @brief Stores the number of command line arguments.
 */
static int app_argc = 0;
/**
 * @internal
 * @brief Stores the command line arguments.
 */
static char **app_argv = NULL;

/**
 * @brief Sets the application arguments.
 *
 * This function stores the command line arguments passed to the application.
 * These arguments can be retrieved later using ecore_app_args_get().
 * It is typically called once at the beginning of the application's execution.
 *
 * @param argc The number of arguments in argv.
 * @param argv An array of strings representing the arguments. The strings
 *             themselves are not copied, only the pointers.
 *             Example:
 *             @code
 *             const char *my_argv[] = { "my_app", "-option", "value", NULL };
 *             int my_argc = 3; // or calculate from my_argv
 *             ecore_app_args_set(my_argc, my_argv);
 *             @endcode
 */
EAPI void
ecore_app_args_set(int          argc,
                   const char **argv)
{
   EINA_MAIN_LOOP_CHECK_RETURN;

   if ((argc < 1) || (!argv))
     {
        if (argc || argv) return;
     }
   app_argc = argc;
   app_argv = (char **)argv;
}

/**
 * @brief Gets the application arguments.
 *
 * This function retrieves the command line arguments previously set by
 * ecore_app_args_set().
 *
 * @param argc A pointer to an integer where the number of arguments will be stored.
 *             If NULL, it is ignored.
 * @param argv A pointer to a char** where the array of argument strings will be stored.
 *             If NULL, it is ignored. The returned array should not be modified.
 *             Example:
 *             @code
 *             int my_argc;
 *             char **my_argv;
 *             ecore_app_args_get(&my_argc, &my_argv);
 *             for (int i = 0; i < my_argc; i++) {
 *               printf("Arg %d: %s\n", i, my_argv[i]);
 *             }
 *             @endcode
 */
EAPI void
ecore_app_args_get(int    *argc,
                   char ***argv)
{
   EINA_MAIN_LOOP_CHECK_RETURN;

   if (argc) *argc = app_argc;
   if (argv) *argv = app_argv;
}

/**
 * @brief Restarts the current application.
 *
 * This function attempts to restart the current application using the arguments
 * originally passed to it (as set by ecore_app_args_set()).
 * This function will not return if successful, as the current process
 * is replaced by a new instance of the application.
 *
 * @note This function relies on the `execvp` system call. If `HAVE_EXECVP`
 *       is not defined, this function will do nothing.
 * @warning This will terminate the current process immediately. Ensure all
 *          necessary cleanup (saving files, releasing resources) is done
 *          before calling this function.
 */
EAPI void
ecore_app_restart(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
#ifdef HAVE_EXECVP
   char *args[4096];
   int i;

   if ((app_argc < 1) || (!app_argv)) return;
   if (app_argc >= 4096) return;
   for (i = 0; i < app_argc; i++) args[i] = app_argv[i];
   args[i] = NULL;
   execvp(app_argv[0], args);
#endif
}
