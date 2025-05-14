/**
 * @file timeout.c
 * @brief Implements a simple timeout mechanism.
 *
 * This file contains the platform-specific implementations for setting up
 * a timeout that executes a callback function and then terminates the program.
 */
#include <stdio.h>

/**
 * @brief Pointer to the function to be called on timeout.
 *
 * This static global variable holds the address of the function
 * that the user wants to execute when the timeout period elapses.
 * It is initialized to NULL, meaning no function will be called by default.
 */
static void (*timeout_func) (void) = NULL;

#ifdef _WIN32
# include <stdio.h>
# include <windows.h>
# include <process.h>

/**
 * @brief Timeout handler function for Windows.
 *
 * This function is executed in a separate thread. It sleeps for the specified
 * duration, then calls the registered timeout function (if any), and finally
 * exits the program.
 *
 * @param arg The timeout duration in seconds, passed as a void pointer.
 * @return unsigned int This function does not normally return as it calls _Exit. Returns 0 if _endthreadex is reached.
 */
unsigned int
_timeout(void *arg)
{
   int s = (int)(uintptr_t)arg;
   Sleep(s * 1000); // Sleep for the specified number of seconds.
   if (timeout_func) timeout_func();
   _Exit(-1);
   _endthreadex(0);
   return 0;
}

/**
 * @brief Initializes the timeout on Windows.
 *
 * Creates a new thread that will execute the _timeout function.
 *
 * @param seconds The number of seconds before the timeout occurs.
 */
void
timeout_init(int seconds)
{
   unsigned int id;
   // Create a new thread that will call _timeout after 'seconds'.
   _beginthreadex( NULL, 0, _timeout, (void *)(uintptr_t)seconds, 0, &id);
}
#else
# include <unistd.h>
# include <signal.h>

/**
 * @brief Timeout handler function for POSIX systems (signal handler for SIGALRM).
 *
 * This function is called when the SIGALRM signal is received.
 * It calls the registered timeout function (if any) and then exits the program.
 *
 * @param val The signal number (unused in this handler, but required by signal handler signature).
 */
static void
_timeout(int val)
{
   if (timeout_func) timeout_func(); // Call the user-defined function if set.
   _exit(-1); // Exit the program.
   if (val) return; // Suppress unused parameter warning, though _exit prevents this from being reached.
}

/**
 * @brief Initializes the timeout on POSIX systems.
 *
 * Sets up a signal handler for SIGALRM and schedules an alarm.
 *
 * @param seconds The number of seconds before the timeout occurs.
 */
void
timeout_init(int seconds)
{
   signal(SIGALRM, _timeout); // Register _timeout as the handler for SIGALRM.
   alarm(seconds); // Schedule SIGALRM to be sent after 'seconds'.
}
#endif

/**
 * @brief Sets the function to be called when the timeout occurs.
 *
 * Assigns the provided function pointer to the global `timeout_func`.
 * This function will be executed just before the program exits due to the timeout.
 *
 * @param func A pointer to a void function that takes no arguments.
 *             This function will be called when the timeout expires.
 *             Pass NULL if no function should be called.
 */
void
timeout_func_set(void (*func) (void))
{
   timeout_func = func;
}
