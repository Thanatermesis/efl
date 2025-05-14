#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>

#include "Ecore.h"
#include "ecore_private.h"

static Eina_Bool _ecore_job_event_handler(void *data,
                                          int   type,
                                          void *ev);
/**
 * @brief Frees the Ecore_Job structure when its associated event is deleted.
 *
 * @param data The Ecore_Job structure to free (passed as @c data to ecore_event_add).
 * @param ev The event structure (unused in this function).
 */
static void _ecore_job_event_free(void *data,
                                  void *ev);

static int ecore_event_job_type = 0; /**< The unique event type ID for Ecore_Job events. */
static Ecore_Event_Handler *_ecore_job_handler = NULL; /**< Handler for Ecore_Job events. */

/**
 * @brief Represents a job to be executed in the Ecore main loop.
 *
 * Jobs are tasks that are scheduled to run once in the main loop,
 * typically after all other events of the current iteration have been processed.
 */
struct _Ecore_Job
{
   Ecore_Event *event; /**< The Ecore_Event associated with this job. */
   Ecore_Cb     func;  /**< The callback function to execute for this job. */
   void        *data;  /**< User-provided data to pass to the callback function. */
};

/**
 * @internal
 * @brief Initializes the Ecore_Job system.
 *
 * This function sets up the event type and handler necessary for Ecore_Job
 * operations. It is called internally by Ecore during initialization.
 */
void
_ecore_job_init(void)
{
   ecore_event_job_type = ecore_event_type_new();
   _ecore_job_handler = ecore_event_handler_add(ecore_event_job_type, _ecore_job_event_handler, NULL);
}

/**
 * @internal
 * @brief Shuts down the Ecore_Job system.
 *
 * This function cleans up resources used by the Ecore_Job system, primarily
 * by deleting the event handler. It is called internally by Ecore during shutdown.
 */
void
_ecore_job_shutdown(void)
{
   ecore_event_handler_del(_ecore_job_handler);
   _ecore_job_handler = NULL;
}

/**
 * @brief Adds a job to the event queue.
 *
 * @param func The function to call when the job is run.
 * @param data The data to pass to the @p func.
 * @return A handle to the new job, or @c NULL on failure.
 *
 * This function schedules the provided @p func to be called later,
 * typically during the next main loop iteration or after other pending
 * events have been processed. The job will only be executed once.
 *
 * Example:
 * @code
 * void my_job_callback(void *data)
 * {
 *    printf("Job executed with data: %s\n", (char *)data);
 * }
 *
 * // ...
 * const char *my_data = "Hello from job";
 * ecore_job_add(my_job_callback, my_data);
 * @endcode
 */
EAPI Ecore_Job *
ecore_job_add(Ecore_Cb    func,
              const void *data)
{
   Ecore_Job *job;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (!func)
     {
        ERR("Callback function must be set up for an Ecore_Job object");
        return NULL;
     }

   job = calloc(1, sizeof (Ecore_Job));
   if (!job) return NULL;

   job->event = ecore_event_add(ecore_event_job_type, job, _ecore_job_event_free, job);
   if (!job->event)
     {
        ERR("No event was assigned to Ecore_Job '%p'", job);
        free(job);
        return NULL;
     }
   job->func = func;
   job->data = (void *)data;

   return job;
}

/**
 * @brief Deletes a job that has been scheduled.
 *
 * @param job The job to delete.
 * @return The data pointer that was passed to ecore_job_add() when the
 *         job was scheduled, or @c NULL if @p job is @c NULL or an error occurs.
 *
 * This function removes the specified @p job from the event queue. If the
 * job has not yet executed, it will be cancelled. The data associated with
 * the job is returned, allowing the caller to free it if necessary.
 *
 * Example:
 * @code
 * Ecore_Job *my_job;
 * const char *my_data = strdup("Important data for job");
 *
 * void my_job_callback(void *data)
 * {
 *    printf("Job executed with data: %s\n", (char *)data);
 *    free(data); // Assuming data was dynamically allocated
 * }
 *
 * my_job = ecore_job_add(my_job_callback, my_data);
 *
 * // ... later, if we need to cancel the job before it runs
 * if (my_job)
 * {
 *    void *cancelled_data = ecore_job_del(my_job);
 *    if (cancelled_data)
 *    {
 *       printf("Job cancelled. Freeing data: %s\n", (char *)cancelled_data);
 *       free(cancelled_data);
 *    }
 * }
 * @endcode
 */
EAPI void *
ecore_job_del(Ecore_Job *job)
{
   void *data;

   if (!job) return NULL;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   data = job->data;
   ecore_event_del(job->event);

   return data;
}

/**
 * @brief Event handler for Ecore_Job events.
 *
 * @param data User data associated with the event handler (unused).
 * @param type The type of the event (unused, always ecore_event_job_type).
 * @param ev The event specific data, which is the Ecore_Job itself.
 * @return ECORE_CALLBACK_DONE to indicate the event is handled and should not propagate.
 *
 * This function is called by the Ecore main loop when a job event is processed.
 * It retrieves the Ecore_Job structure from the event and executes the
 * job's callback function.
 */
static Eina_Bool
_ecore_job_event_handler(void *data EINA_UNUSED,
                         int   type EINA_UNUSED,
                         void *ev)
{
   Ecore_Job *job;

   job = ev;
   job->func(job->data);
   return ECORE_CALLBACK_DONE;
}

static void
_ecore_job_event_free(void *data,
                      void *job EINA_UNUSED)
{
   free(data);
}
