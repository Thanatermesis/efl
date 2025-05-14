#ifndef _EO_ADD_FALLBACK_H
#define _EO_ADD_FALLBACK_H

#include <Eina.h>
#include <Eo.h>

/**
 * @brief Represents a single frame in the Eo call stack.
 *
 * This structure holds a pointer to an Eo object, which is relevant
 * for the context of a specific call in the fallback mechanism.
 */
typedef struct _Eo_Stack_Frame
{
   Eo *obj; /**< Pointer to the Eo object for this stack frame. */
} Eo_Stack_Frame;

/**
 * @brief Initializes the efl_add fallback mechanism.
 *
 * This function sets up the necessary resources, such as TLS keys and
 * the main loop's call stack, for the fallback system to operate.
 * It should be called during EFL initialization.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool _efl_add_fallback_init(void);

/**
 * @brief Shuts down the efl_add fallback mechanism.
 *
 * This function releases resources allocated by _efl_add_fallback_init(),
 * including TLS keys and the main loop's call stack.
 * It should be called during EFL shutdown.
 *
 * @return @c EINA_TRUE on success (currently always returns EINA_TRUE).
 */
Eina_Bool _efl_add_fallback_shutdown(void);

/**
 * @brief Pushes a new frame onto the Eo call stack for the current thread.
 *
 * This is used when an Eo object is being added, to keep track of the
 * context in case a fallback to a simpler add mechanism is needed.
 *
 * @param obj The Eo object to associate with the new stack frame.
 * @return A pointer to the newly pushed stack frame, or behavior is undefined
 *         on stack overflow (results in a critical error).
 */
Eo_Stack_Frame *_efl_add_fallback_stack_push(Eo *obj);

/**
 * @brief Pops the current frame from the Eo call stack for the current thread.
 *
 * This is used when the process of adding an Eo object is complete or aborted.
 *
 * @return A pointer to the stack frame that is now current after the pop,
 *         or behavior is undefined on stack underflow (results in a critical error).
 */
Eo_Stack_Frame *_efl_add_fallback_stack_pop(void);

#endif
