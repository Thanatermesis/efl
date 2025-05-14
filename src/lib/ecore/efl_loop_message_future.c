#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_MESSAGE_FUTURE_CLASS

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Private data for the Efl_Loop_Message_Future class.
 *
 * This structure holds the data associated with an instance of
 * Efl_Loop_Message_Future.
 */
typedef struct _Efl_Loop_Message_Future_Data Efl_Loop_Message_Future_Data;

struct _Efl_Loop_Message_Future_Data
{
   void *data; /**< User-provided data to be associated with the future message. */
};

//////////////////////////////////////////////////////////////////////////

EOLIAN static void
_efl_loop_message_future_data_set(Eo *obj EINA_UNUSED, Efl_Loop_Message_Future_Data *pd, void *data)
{
   pd->data = data;
}

EOLIAN static void *
_efl_loop_message_future_data_get(const Eo *obj EINA_UNUSED, Efl_Loop_Message_Future_Data *pd)
{
   return pd->data;
}

/**
 * @internal
 * @brief Constructor for the Efl_Loop_Message_Future object.
 *
 * This function is called when a new Efl_Loop_Message_Future object is
 * created. It chains up to the parent class constructor.
 *
 * @param obj The Efl_Loop_Message_Future object being constructed.
 * @param pd The private data for the Efl_Loop_Message_Future object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_loop_message_future_efl_object_constructor(Eo *obj, Efl_Loop_Message_Future_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl_Loop_Message_Future object.
 *
 * This function is called when an Efl_Loop_Message_Future object is
 * being destroyed. It chains up to the parent class destructor.
 *
 * @param obj The Efl_Loop_Message_Future object being destructed.
 * @param pd The private data for the Efl_Loop_Message_Future object.
 */
EOLIAN static void
_efl_loop_message_future_efl_object_destructor(Eo *obj, Efl_Loop_Message_Future_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

//////////////////////////////////////////////////////////////////////////

#include "efl_loop_message_future.eo.c"
