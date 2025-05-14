#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

typedef struct _Efl_Loop_Consumer_Data Efl_Loop_Consumer_Data;
struct _Efl_Loop_Consumer_Data
{
};

/**
 * @brief Retrieves the Efl_Loop associated with the given Efl_Loop_Consumer object.
 *
 * This function first attempts to find an Efl_Loop provider for the object.
 * If no provider is found and the main loop is running, it falls back to
 * returning the main loop. An error is logged in this fallback case.
 *
 * @param obj The Efl_Loop_Consumer object.
 * @param pd Private data for the Efl_Loop_Consumer object (unused).
 * @return The associated Efl_Loop, or the main loop as a fallback, or NULL if neither is found.
 */
static Efl_Loop *
_efl_loop_consumer_loop_get(const Eo *obj, Efl_Loop_Consumer_Data *pd EINA_UNUSED)
{
   Efl_Loop *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   if (!loop && eina_main_loop_is())
     {
        loop = efl_main_loop_get();
        ERR("Failed to get the loop on object %p from the main thread! "
            "Returning the main loop: %p", obj, loop);
     }
   return loop;
}

/**
 * @brief Sets the parent of the Efl_Loop_Consumer object.
 *
 * This function checks if the provided parent object is a provider of EFL_LOOP_CLASS.
 * If the parent is not NULL and does not provide EFL_LOOP_CLASS, an error is logged,
 * and the parent is not set. Otherwise, the parent is set using the superclass's
 * efl_parent_set function.
 *
 * @param obj The Efl_Loop_Consumer object.
 * @param pd Private data for the Efl_Loop_Consumer object (unused).
 * @param parent The Efl_Object to set as the parent.
 */
static void
_efl_loop_consumer_efl_object_parent_set(Eo *obj, Efl_Loop_Consumer_Data *pd EINA_UNUSED, Efl_Object *parent)
{
   if (parent != NULL && efl_provider_find(parent, EFL_LOOP_CLASS) == NULL)
     {
        ERR("parent=%p is not a provider of EFL_LOOP_CLASS!", parent);
        return;
     }

   efl_parent_set(efl_super(obj, EFL_LOOP_CONSUMER_CLASS), parent);
}

/**
 * @brief Creates a new resolved Eina_Future associated with the object's loop scheduler.
 *
 * @param obj The Efl_Loop_Consumer object.
 * @param pd Private data for the Efl_Loop_Consumer object (unused).
 * @param result The Eina_Value with which the future will be resolved.
 * @return A new Eina_Future that is already resolved with the given result.
 */
static Eina_Future *
_efl_loop_consumer_future_resolved(const Eo *obj, Efl_Loop_Consumer_Data *pd EINA_UNUSED,
                                   Eina_Value result)
{
   return eina_future_resolved(efl_loop_future_scheduler_get(obj), result);
}

/**
 * @brief Creates a new rejected Eina_Future associated with the object's loop scheduler.
 *
 * @param obj The Efl_Loop_Consumer object.
 * @param pd Private data for the Efl_Loop_Consumer object (unused).
 * @param error The Eina_Error with which the future will be rejected.
 * @return A new Eina_Future that is already rejected with the given error.
 */
static Eina_Future *
_efl_loop_consumer_future_rejected(const Eo *obj, Efl_Loop_Consumer_Data *pd EINA_UNUSED,
                                   Eina_Error error)
{
   return eina_future_rejected(efl_loop_future_scheduler_get(obj), error);
}

/**
 * @brief A dummy cancellation function for Eina_Promise.
 *
 * This function does nothing when a promise is cancelled. It's used when
 * creating promises that don't require specific cleanup on cancellation.
 *
 * @param data User data associated with the promise (unused).
 * @param p The Eina_Promise being cancelled (unused).
 */
static void
_dummy_cancel(void *data EINA_UNUSED, const Eina_Promise *p EINA_UNUSED)
{
}

/**
 * @brief Creates a new Eina_Promise associated with the object's loop scheduler.
 *
 * The created promise uses a dummy cancellation function.
 *
 * @param obj The Efl_Loop_Consumer object.
 * @param pd Private data for the Efl_Loop_Consumer object (unused).
 * @return A new Eina_Promise.
 */
static Eina_Promise *
_efl_loop_consumer_promise_new(const Eo *obj, Efl_Loop_Consumer_Data *pd EINA_UNUSED)
{
   return eina_promise_new(efl_loop_future_scheduler_get(obj), _dummy_cancel, NULL);
}

#include "efl_loop_consumer.eo.c"
