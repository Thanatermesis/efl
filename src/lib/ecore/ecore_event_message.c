#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS ECORE_EVENT_MESSAGE_CLASS

//////////////////////////////////////////////////////////////////////////

typedef struct _Ecore_Event_Message_Data Ecore_Event_Message_Data;

/**
 * @internal
 * @brief Structure holding the data for an Ecore_Event_Message.
 *
 * This structure contains the actual event data, its type, and information
 * on how to free the event data when the message is no longer needed.
 */
struct _Ecore_Event_Message_Data
{
   int type;              /**< The type of the event. This typically corresponds to an ECORE_EVENT_* type. */
   void *ev;              /**< Pointer to the actual event data. This is cast to the appropriate event structure based on the type. */
   Ecore_End_Cb free_func; /**< Optional callback function to free the event data pointed to by @p ev. If NULL, free() will be used. */
   void *data;            /**< User data to be passed to the @p free_func when it's called. */
};

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Sets the data for an Ecore_Event_Message.
 *
 * This function populates the Ecore_Event_Message_Data structure with the
 * provided event information.
 *
 * @param obj The Efl_Object instance (unused).
 * @param pd Pointer to the Ecore_Event_Message_Data structure to populate.
 * @param type The event type.
 * @param data Pointer to the event data.
 * @param free_func Optional callback to free the event data.
 * @param free_data User data for the free_func.
 */
EOLIAN static void
_ecore_event_message_data_set(Eo *obj EINA_UNUSED, Ecore_Event_Message_Data *pd, int type, void *data, void *free_func, void *free_data)
{
   pd->type = type;
   pd->ev = data;
   pd->free_func = free_func;
   pd->data = free_data;
}

/**
 * @internal
 * @brief Retrieves the data from an Ecore_Event_Message.
 *
 * This function copies the event information from the Ecore_Event_Message_Data
 * structure to the provided output parameters.
 *
 * @param obj The Efl_Object instance (unused).
 * @param pd Pointer to the Ecore_Event_Message_Data structure.
 * @param type Pointer to store the event type. Can be NULL.
 * @param data Pointer to store the event data. Can be NULL.
 * @param free_func Pointer to store the free function. Can be NULL.
 * @param free_data Pointer to store the user data for the free function. Can be NULL.
 */
EOLIAN static void
_ecore_event_message_data_get(const Eo *obj EINA_UNUSED, Ecore_Event_Message_Data *pd, int *type, void **data, void **free_func, void **free_data)
{
   if (type) *type = pd->type;
   if (data) *data = pd->ev;
   if (free_func) *free_func = pd->free_func;
   if (free_data) *free_data = pd->data;
}

/**
 * @internal
 * @brief Retrieves and clears the data from an Ecore_Event_Message.
 *
 * This function copies the event information from the Ecore_Event_Message_Data
 * structure to the provided output parameters and then clears the internal
 * data, effectively transferring ownership of the event data to the caller.
 * The message object will no longer manage the lifetime of the event data.
 *
 * @param obj The Efl_Object instance (unused).
 * @param pd Pointer to the Ecore_Event_Message_Data structure.
 * @param type Pointer to store the event type. Can be NULL.
 * @param data Pointer to store the event data. Can be NULL.
 * @param free_func Pointer to store the free function. Can be NULL.
 * @param free_data Pointer to store the user data for the free function. Can be NULL.
 */
EOLIAN static void
_ecore_event_message_data_steal(Eo *obj EINA_UNUSED, Ecore_Event_Message_Data *pd, int *type, void **data, void **free_func, void **free_data)
{
   if (type) *type = pd->type;
   if (data) *data = pd->ev;
   if (free_func) *free_func = pd->free_func;
   if (free_data) *free_data = pd->data;
   pd->type = -1;
   pd->ev = NULL;
   pd->free_func = NULL;
   pd->data = NULL;
}

/**
 * @internal
 * @brief Constructor for the Ecore_Event_Message object.
 *
 * Initializes the Ecore_Event_Message object by calling the parent constructor
 * and setting the initial event type to -1 (invalid).
 *
 * @param obj The Efl_Object instance being constructed.
 * @param pd Pointer to the Ecore_Event_Message_Data structure (unused in this specific constructor logic beyond initialization).
 * @return The constructed Efl_Object instance.
 */
EOLIAN static Efl_Object *
_ecore_event_message_efl_object_constructor(Eo *obj, Ecore_Event_Message_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->type = -1;
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ecore_Event_Message object.
 *
 * Frees the event data associated with the message if it exists. It uses
 * the provided free_func if available, otherwise, it defaults to free().
 * After freeing the event data, it calls the parent destructor.
 *
 * @param obj The Efl_Object instance being destructed (unused).
 * @param pd Pointer to the Ecore_Event_Message_Data structure.
 */
EOLIAN static void
_ecore_event_message_efl_object_destructor(Eo *obj EINA_UNUSED, Ecore_Event_Message_Data *pd EINA_UNUSED)
{
   if (pd->ev)
     {
        Ecore_End_Cb fn_free = pd->free_func;
        void *ev = pd->ev;

        pd->ev = NULL;
        if (fn_free) fn_free(pd->data, ev);
        else free(ev);
     }
   efl_destructor(efl_super(obj, MY_CLASS));
}

//////////////////////////////////////////////////////////////////////////

#include "ecore_event_message.eo.c"
