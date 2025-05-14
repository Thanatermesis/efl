#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef __linux__
# include <sys/syscall.h>
#endif
#include "evas_common_private.h"
#include "evas_private.h"
#include "Evas.h"

#include "Ecore.h"

/**
 * @internal
 * @brief A function pointer type for preload thread operations.
 * @param data User data to be passed to the function.
 */
typedef void (*_evas_preload_pthread_func)(void *data);

/**
 * @internal
 * @brief Structure to manage a preloading task in a separate thread.
 *
 * This structure holds all necessary information for a preloading operation,
 * including the functions to execute for the heavy lifting, completion,
 * and cancellation, as well as the associated Ecore_Thread.
 */
typedef struct _Evas_Preload_Pthread Evas_Preload_Pthread;
struct _Evas_Preload_Pthread
{
   EINA_INLIST; /**< Macro to make this struct usable with Eina_Inlist */

   Ecore_Thread *thread; /**< The Ecore thread handling this preload task */

   _evas_preload_pthread_func func_heavy; /**< Function to execute in the thread (heavy task) */
   _evas_preload_pthread_func func_end; /**< Function to execute on successful completion in the main loop */
   _evas_preload_pthread_func func_cancel; /**< Function to execute on cancellation/failure in the main loop */
   void *data; /**< User data passed to the callback functions */
};

/**
 * @internal
 * @brief List of active preloading works.
 */
static Eina_Inlist *works = NULL;

/**
 * @internal
 * @brief Frees an Evas_Preload_Pthread structure and removes it from the active list.
 * @param work The preloading work structure to free.
 */
static void
_evas_preload_thread_work_free(Evas_Preload_Pthread *work)
{
   works = eina_inlist_remove(works, EINA_INLIST_GET(work));

   free(work);
}

/**
 * @internal
 * @brief Callback executed in the main loop when a preloading thread successfully completes.
 * @param data The Evas_Preload_Pthread work structure.
 * @param thread The Ecore_Thread that completed (unused).
 */
static void
_evas_preload_thread_success(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Evas_Preload_Pthread *work = data;

   work->func_end(work->data);

    _evas_preload_thread_work_free(work);
}

/**
 * @internal
 * @brief Callback executed in the main loop when a preloading thread fails or is cancelled.
 * @param data The Evas_Preload_Pthread work structure.
 * @param thread The Ecore_Thread that failed or was cancelled (unused).
 */
static void
_evas_preload_thread_fail(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Evas_Preload_Pthread *work = data;

   if (work->func_cancel) work->func_cancel(work->data);

   _evas_preload_thread_work_free(work);
}

/**
 * @internal
 * @brief The main worker function executed in the separate preloading thread.
 * @param data The Evas_Preload_Pthread work structure.
 * @param thread The Ecore_Thread in which this function is running.
 */
static void
_evas_preload_thread_worker(void *data, Ecore_Thread *thread)
{
   Evas_Preload_Pthread *work = data;

   work->thread = thread;

   work->func_heavy(work->data);
}

/**
 * @internal
 * @brief Initializes the Evas preloading thread system.
 *
 * Currently, this function does nothing but is kept for API consistency
 * and potential future use.
 */
void
_evas_preload_thread_init(void)
{
}

/**
 * @internal
 * @brief Shuts down the Evas preloading thread system.
 *
 * This function attempts to cancel all ongoing preloading threads and waits
 * for them to complete. If threads do not complete within a timeout,
 * an error is logged.
 */
void
_evas_preload_thread_shutdown(void)
{
   Evas_Preload_Pthread *work;

   EINA_INLIST_FOREACH(works, work)
     ecore_thread_cancel(work->thread);

   while (works)
     {
        work = (Evas_Preload_Pthread*) works;
        if (!ecore_thread_wait(work->thread, 1))
          {
             ERR("Can not wait any longer on Evas thread to be done during shutdown. This might lead to a crash.");
             works = eina_inlist_remove(works, works);
          }
     }
}

/**
 * @brief Runs a function in a separate thread for preloading.
 *
 * This function creates a new thread to execute @p func_heavy.
 * On successful completion, @p func_end is called in the main loop.
 * If the thread is cancelled or fails, @p func_cancel is called in the main loop.
 *
 * @param func_heavy The function to execute in the new thread. This function
 *                   will perform the time-consuming preloading task.
 *                   Example: `void my_heavy_loader(void *my_data) { // load resources }`
 * @param func_end The function to call in the main loop when @p func_heavy completes.
 *                 Example: `void my_loader_done(void *my_data) { // use loaded resources }`
 * @param func_cancel The function to call in the main loop if the thread is cancelled
 *                    or if @p func_heavy encounters an error (not directly, but if thread fails).
 *                    Also called if memory allocation for the work structure fails.
 *                    Example: `void my_loader_cancel(void *my_data) { // cleanup }`
 * @param data Custom data pointer to be passed to @p func_heavy, @p func_end, and @p func_cancel.
 * @return A pointer to an Evas_Preload_Pthread structure representing the work,
 *         or @c NULL on failure (e.g., memory allocation failure or thread creation failure).
 */
Evas_Preload_Pthread *
evas_preload_thread_run(void (*func_heavy) (void *data),
                        void (*func_end) (void *data),
                        void (*func_cancel) (void *data),
                        const void *data)
{
   Evas_Preload_Pthread *work;

   work = malloc(sizeof(Evas_Preload_Pthread));
   if (!work)
     {
        func_cancel((void *)data);
        return NULL;
     }

   work->func_heavy = func_heavy;
   work->func_end = func_end;
   work->func_cancel = func_cancel;
   work->data = (void *)data;

   work->thread = ecore_thread_run(_evas_preload_thread_worker,
                                   _evas_preload_thread_success,
                                   _evas_preload_thread_fail,
                                   work);
   if (!work->thread)
     return NULL;

   works = eina_inlist_prepend(works, EINA_INLIST_GET(work));

   return work;
}

/**
 * @brief Attempts to cancel an ongoing preloading thread.
 *
 * @param work The preloading work structure, as returned by evas_preload_thread_run().
 * @return @c EINA_TRUE if the cancel request was successfully sent, @c EINA_FALSE otherwise.
 *         Note that this does not guarantee the thread will be cancelled immediately or at all,
 *         as cancellation depends on the Ecore_Thread implementation and the cooperation
 *         of the worker function.
 */
Eina_Bool
evas_preload_thread_cancel(Evas_Preload_Pthread *work)
{
   return ecore_thread_cancel(work->thread);
}

/**
 * @brief Checks if a preloading thread has been cancelled.
 *
 * This function checks the status of the Ecore thread to see if it has been
 * marked for cancellation or has already exited due to cancellation.
 *
 * @param work The preloading work structure. If NULL, returns EINA_FALSE.
 * @return @c EINA_TRUE if the thread associated with @p work has been cancelled,
 *         @c EINA_FALSE otherwise or if @p work is @c NULL.
 */
Eina_Bool
evas_preload_thread_cancelled_is(Evas_Preload_Pthread *work)
{
   if (!work) return EINA_FALSE;
   return ecore_thread_check(work->thread);
}

/**
 * @brief Waits for a preloading thread to complete.
 *
 * This function blocks the calling (main) thread until the specified preloading
 * thread (@p work) finishes its execution or the @p wait timeout occurs.
 * It integrates with the Ecore main loop.
 *
 * @param work The preloading work structure. If NULL, returns EINA_TRUE immediately.
 * @param wait The maximum time in seconds to wait for the thread to complete.
 *             A value of 0 means wait indefinitely.
 * @return @c EINA_TRUE if the thread completed (or if @p work was NULL),
 *         @c EINA_FALSE if the timeout was reached before the thread completed.
 */
Eina_Bool
evas_preload_pthread_wait(Evas_Preload_Pthread *work, double wait)
{
   Eina_Bool r;

   if (!work) return EINA_TRUE;

   ecore_thread_main_loop_begin();
   r = ecore_thread_wait(work->thread, wait);
   ecore_thread_main_loop_end();

   return r;
}
