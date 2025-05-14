/**
 * @file timeout.h
 * @brief Provides a simple timeout mechanism.
 *
 * This header declares functions to initialize a timeout that, upon expiring,
 * calls a user-defined function and then exits the program.
 * It supports both Windows and POSIX-like systems.
 */
#ifndef TIMEOUT_H
#define TIMEOUT_H 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the timeout.
 *
 * Sets up a timer that will trigger after the specified number of seconds.
 * On POSIX systems, this uses `alarm()` and `SIGALRM`.
 * On Windows, this creates a new thread that sleeps and then calls the timeout function.
 * After the timeout function (if any) is called, the program will exit with status -1.
 *
 * @param seconds The number of seconds before the timeout occurs.
 *                Example: `timeout_init(30);` // Timeout after 30 seconds.
 */
void timeout_init(int seconds);

/**
 * @brief Sets the function to be called when the timeout occurs.
 *
 * The provided function will be executed just before the program exits due to the timeout.
 *
 * @param func A pointer to a void function that takes no arguments.
 *             This function will be called when the timeout expires.
 *             Pass NULL if no function should be called.
 *             Example: `void my_cleanup_func(void) { printf("Timeout occurred!\n"); }`
 *                      `timeout_func_set(my_cleanup_func);`
 */
void timeout_func_set(void (*func) (void));

#ifdef __cplusplus
}
#endif

#endif
