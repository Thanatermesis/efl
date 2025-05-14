#include "edje_private.h"

static void _edje_message_propagate_send(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id, void *emsg, Eina_Bool prop);

static int _injob = 0; /**< Counter to track if we are currently inside an Ecore_Job execution. */
static Ecore_Job *_job = NULL; /**< Ecore_Job used to process the message queue asynchronously. */
static Ecore_Timer *_job_loss_timer = NULL; /**< Timer to ensure job processing isn't lost if events are missed. */

static Eina_Inlist *msgq = NULL; /**< Main message queue for Edje messages. */
static Eina_Inlist *tmp_msgq = NULL; /**< Temporary message queue used during processing to handle re-entrant calls. */
static int tmp_msgq_processing = 0; /**< Flag indicating if the temporary message queue is currently being processed. */
static int tmp_msgq_restart = 0; /**< Flag to indicate if processing of the temporary queue needs to be restarted. */

static Eina_Inlist *_edje_msg_trash = NULL; /**< A list of freed Edje_Message structures for reuse, to reduce malloc/free overhead. */

/*============================================================================*
*                                   API                                      *
*============================================================================*/

#define INLIST_CONTAINER(container_type, list, list_entry) \
   (container_type *)((unsigned char *)list - offsetof(container_type, list_entry))

/**
 * @brief Pops an Edje_Message from the trash list for reuse.
 *
 * This function retrieves a pre-allocated Edje_Message structure from a
 * cache (_edje_msg_trash) to avoid repeated malloc calls. If the trash is
 * empty, it returns NULL.
 *
 * @return A pointer to an Edje_Message structure, or NULL if the trash is empty.
 */
static Edje_Message *
_edje_msg_trash_pop(void)
{
   Edje_Message *em;

   if (!_edje_msg_trash) return NULL;
   em = INLIST_CONTAINER(Edje_Message, _edje_msg_trash, inlist_main);
   _edje_msg_trash = eina_inlist_remove(_edje_msg_trash, &(em->inlist_main));
   memset(em, 0, sizeof(Edje_Message));
   return em;
}

/**
 * @brief Pushes an Edje_Message to the trash list for future reuse.
 *
 * This function adds an Edje_Message structure (that is no longer needed
 * immediately) to a cache (_edje_msg_trash) to make it available for
 * quick reuse later, reducing free/malloc overhead.
 *
 * @param em The Edje_Message to add to the trash.
 */
static void
_edje_msg_trash_push(Edje_Message *em)
{
   _edje_msg_trash = eina_inlist_prepend(_edje_msg_trash, &(em->inlist_main));
}

/**
 * @brief Clears all Edje_Message structures from the trash list.
 *
 * This function iterates through the _edje_msg_trash list, frees each
 * Edje_Message structure, and empties the list. This is typically called
 * during shutdown.
 */
static void
_edje_msg_trash_clear(void)
{
   Edje_Message *em;

   while (_edje_msg_trash)
     {
        em = _edje_msg_trash_pop();
        free(em);
     }
}

/**
 * @brief Sends a message to an Edje object and optionally propagates it to its subobjects.
 *
 * This function fetches the Edje data associated with the Evas_Object,
 * then calls _edje_message_propagate_send to queue the message.
 * If propagation is enabled (implicitly for subobjects here), it recursively
 * calls itself for all subobjects.
 *
 * @param obj The Evas_Object (Edje object) to send the message to.
 * @param type The type of the message.
 * @param id The ID of the message.
 * @param msg A pointer to the message data. The structure of this data depends on the message type.
 * @param prop EINA_TRUE to indicate this is a propagated message (used internally for subobjects).
 */
static void
_edje_object_message_propagate_send(Evas_Object *obj, Edje_Message_Type type, int id, void *msg, Eina_Bool prop)
{
   Edje *ed;
   Eina_List *l;
   Evas_Object *o;

   ed = _edje_fetch(obj);
   if (!ed) return;
   _edje_message_propagate_send(ed, EDJE_QUEUE_SCRIPT, type, id, msg, prop);
   EINA_LIST_FOREACH(ed->subobjs, l, o)
     {
        _edje_object_message_propagate_send(o, type, id, msg, EINA_TRUE);
     }
}

EOLIAN void
_efl_canvas_layout_efl_layout_signal_message_send(Eo *obj, Edje *pd EINA_UNUSED, int id, const Eina_Value val)
{
   const Eina_Value_Type *valtype;
   Edje_Message_Type msgtype;

   /* Note: Only primitive types & arrays of them are supported.
    * This reduces complexity and I couldn't find many real uses for combo
    * types (string+int or string+float).
    */

   union {
      Edje_Message_String str;
      Edje_Message_Int i;
      Edje_Message_Float f;
      Edje_Message_String_Set ss;
      Edje_Message_Int_Set is;
      Edje_Message_Float_Set fs;
      //Edje_Message_String_Int si;
      //Edje_Message_String_Float sf;
      //Edje_Message_String_Int_Set sis;
      //Edje_Message_String_Float_Set sfs;
   } msg, *pmsg;

   valtype = eina_value_type_get(&val);
   if (!valtype) goto bad_type;

   pmsg = &msg;
   if ((valtype == EINA_VALUE_TYPE_STRING) ||
       (valtype == EINA_VALUE_TYPE_STRINGSHARE))
     {
        eina_value_get(&val, &msg.str.str);
        msgtype = EDJE_MESSAGE_STRING;
     }
   else if (valtype == EINA_VALUE_TYPE_INT)
     {
        eina_value_get(&val, &msg.i.val);
        msgtype = EDJE_MESSAGE_INT;
     }
   else if (valtype == EINA_VALUE_TYPE_FLOAT)
     {
        float f;
        eina_value_get(&val, &f);
        msg.f.val = (double) f;
        msgtype = EDJE_MESSAGE_FLOAT;
     }
   else if (valtype == EINA_VALUE_TYPE_DOUBLE)
     {
        eina_value_get(&val, &msg.f.val);
        msgtype = EDJE_MESSAGE_FLOAT;
     }
   else if (valtype == EINA_VALUE_TYPE_ARRAY)
     {
        Eina_Value_Array array = {};
        size_t sz, k, count;

        eina_value_get(&val, &array);
        count = eina_inarray_count(array.array);
        if ((array.subtype == EINA_VALUE_TYPE_STRING) ||
            (array.subtype == EINA_VALUE_TYPE_STRINGSHARE))
          {
             sz = sizeof(char *);
             msgtype = EDJE_MESSAGE_STRING_SET;
             pmsg = alloca(sizeof(*pmsg) + sz * count);
             pmsg->ss.count = count;
             for (k = 0; k < count; k++)
               pmsg->ss.str[k] = eina_inarray_nth(array.array, k);
          }
        else if (array.subtype == EINA_VALUE_TYPE_INT)
          {
             sz = sizeof(int);
             msgtype = EDJE_MESSAGE_INT_SET;
             pmsg = alloca(sizeof(*pmsg) + sz * count);
             pmsg->is.count = count;
             for (k = 0; k < count; k++)
               pmsg->is.val[k] = *((int *) eina_inarray_nth(array.array, k));
          }
        else if (array.subtype == EINA_VALUE_TYPE_DOUBLE)
          {
             sz = sizeof(double);
             msgtype = EDJE_MESSAGE_FLOAT_SET;
             pmsg = alloca(sizeof(*pmsg) + sz * count);
             pmsg->fs.count = count;
             for (k = 0; k < count; k++)
               pmsg->fs.val[k] = *((double *) eina_inarray_nth(array.array, k));
          }
        else if (array.subtype == EINA_VALUE_TYPE_FLOAT)
          {
             sz = sizeof(double);
             msgtype = EDJE_MESSAGE_FLOAT_SET;
             pmsg = alloca(sizeof(*pmsg) + sz * count);
             pmsg->fs.count = count;
             for (k = 0; k < count; k++)
               pmsg->fs.val[k] = (double) *((float *) eina_inarray_nth(array.array, k));
          }
        else goto bad_type;

     }
   else goto bad_type;

   _edje_object_message_propagate_send(obj, msgtype, id, pmsg, EINA_FALSE);
   return;

bad_type:
   ERR("Unsupported value type: %s. Only primitives types int, real "
       "(float or double), string or arrays of those types are supported.",
       eina_value_type_name_get(valtype));
   return;
}

/**
 * @brief Processes messages for a specific Edje object, including those from its group.
 *
 * This function iterates through messages queued for the given Edje object (`ed`)
 * and its associated groups. It moves relevant messages from the global queue (`msgq`)
 * to a temporary queue (`tmp_msgq`) for processing. It handles potential
 * re-entrancy and message loops by using `tmp_msgq_restart` and a goto limit.
 *
 * @param obj The Evas_Object associated with the Edje instance (currently unused in this function).
 * @param ed The Edje instance whose messages are to be processed.
 */
static void
_edje_object_message_signal_process_do(Eo *obj EINA_UNUSED, Edje *ed)
{
   Eina_Inlist *l, *ln;
   Edje *lookup_ed = NULL;
   Eina_List *groups = NULL, *lg;
   Edje_Message *em;
   int gotos = 0;

   if (!ed) return;

   groups = ed->groups;
   if (groups)
     {
        for (l = msgq; l; l = ln)
          {
             ln = l->next;
             em = INLIST_CONTAINER(Edje_Message, l, inlist_main);
             EINA_LIST_FOREACH(groups, lg, lookup_ed)
               {
                  if (em->edje == lookup_ed)
                    {
                       msgq = eina_inlist_remove(msgq, &(em->inlist_main));
                       tmp_msgq = eina_inlist_append(tmp_msgq, &(em->inlist_main));
                       em->in_tmp_msgq = EINA_TRUE;
                       break;
                    }
               }
          }
     }

   tmp_msgq_processing++;
again:
   for (l = ed->messages; l; l = ln)
     {
        ln = l->next;
        em = INLIST_CONTAINER(Edje_Message, l, inlist_edje);
        if (!em->in_tmp_msgq) continue;
        // so why this? any group edje is not the parent - skip this
        lookup_ed = NULL;
        EINA_LIST_FOREACH(groups, lg, lookup_ed)
          {
             if (em->edje == lookup_ed) break;
          }
        if (!lookup_ed) continue;
        tmp_msgq = eina_inlist_remove(tmp_msgq, &(em->inlist_main));
        lookup_ed->messages = eina_inlist_remove(lookup_ed->messages, &(em->inlist_edje));
        lookup_ed->message.num--;
        if (!lookup_ed->delete_me)
          {
             lookup_ed->processing_messages++;
             _edje_message_process(em);
             _edje_message_free(em);
             lookup_ed->processing_messages--;
          }
        else
          _edje_message_free(em);
        if (lookup_ed->processing_messages == 0)
          {
             if (lookup_ed->delete_me) _edje_del(lookup_ed);
          }
        // if some child callback in _edje_message_process called
        // edje_object_message_signal_process() or
        // edje_message_signal_process() then those will mark the restart
        // flag when they finish - it mabsicammyt means tmp_msgq and
        // any item in it has potentially become invalid - so that means l
        // and ln could be rogue pointers, so start again from the beginning
        // and skip anything that is not this object and process only what is.
        // to avoid self-feeding loops allow a max of 1024 loops.
        if (tmp_msgq_restart)
          {
             tmp_msgq_restart = 0;
             gotos++;
             if (gotos < 1024) goto again;
             else
               {
                  WRN("Edje is in a self-feeding message loop (> 1024 gotos needed in a row)");
                  goto end;
               }
          }
     }
end:
   tmp_msgq_processing--;
   if (tmp_msgq_processing == 0)
     tmp_msgq_restart = 0;
   else
     tmp_msgq_restart = 1;
}

EOLIAN void
_efl_canvas_layout_efl_layout_signal_signal_process(Eo *obj, Edje *ed, Eina_Bool recurse)
{
   Eina_List *l;
   Evas_Object *o;

   if (ed->delete_me) return;
   _edje_object_message_signal_process_do(obj, ed);
   if (!recurse) return;

   EINA_LIST_FOREACH(ed->subobjs, l, o)
     efl_layout_signal_process(o, EINA_TRUE);
}

static Eina_Bool
_edje_dummy_timer(void *data EINA_UNUSED)
{
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Ecore_Job callback function to process the Edje message queue.
 *
 * This function is scheduled as an Ecore_Job to process messages
 * asynchronously. It ensures that any job loss timer is cleared,
 * processes the message queue, clears the message trash, and handles
 * re-entrancy using the _injob counter.
 *
 * @param data User data passed to the job function (unused).
 */
static void
_edje_job(void *data EINA_UNUSED)
{
   if (_job_loss_timer)
     {
        ecore_timer_del(_job_loss_timer);
        _job_loss_timer = NULL;
     }
   _job = NULL;
   _injob++;
   _edje_message_queue_process();
   _edje_msg_trash_clear();
   _injob--;
}

/**
 * @brief Ecore_Timer callback to safeguard against job loss.
 *
 * If the main job (`_edje_job`) was expected to run but didn't (e.g., due
 * to event processing issues), this timer ensures that a new job is added
 * to process the Edje messages.
 *
 * @param data User data passed to the timer callback (unused).
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer after it fires.
 */
static Eina_Bool
_edje_job_loss_timer(void *data EINA_UNUSED)
{
   _job_loss_timer = NULL;
   if (!_job)
     {
        _job = ecore_job_add(_edje_job, NULL);
     }
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Initializes the Edje message subsystem.
 *
 * Currently, this function is a placeholder and does not perform any
 * specific initialization tasks.
 */
void
_edje_message_init(void)
{
}

/**
 * @brief Shuts down the Edje message subsystem.
 *
 * This function clears all pending messages from the queues, cleans up
 * the message trash, and deletes any active Ecore_Timer or Ecore_Job
 * related to message processing.
 */
void
_edje_message_shutdown(void)
{
   _edje_message_queue_clear();
   _edje_msg_trash_clear();
   if (_job_loss_timer)
     {
        ecore_timer_del(_job_loss_timer);
        _job_loss_timer = NULL;
     }
   if (_job)
     {
        ecore_job_del(_job);
        _job = NULL;
     }
}

/**
 * @brief Sets the message handler callback for an Edje object and its sub-objects.
 *
 * This function registers a callback function (`func`) and associated user data (`data`)
 * to be invoked when an Edje object receives a message of type EDJE_QUEUE_APP.
 * The callback is also recursively set for all sub-objects of the given Edje object.
 *
 * @param ed The Edje object to set the message handler for.
 * @param func The callback function to handle messages.
 *             It takes the following arguments:
 *             - `void *data`: The user data provided when setting the callback.
 *             - `Evas_Object *obj`: The Edje Evas_Object that received the message.
 *             - `Edje_Message_Type type`: The type of the message.
 *             - `int id`: The ID of the message.
 *             - `void *msg`: A pointer to the message data.
 * @param data User data to be passed to the callback function.
 */
void
_edje_message_cb_set(Edje *ed, void (*func)(void *data, Evas_Object *obj, Edje_Message_Type type, int id, void *msg), void *data)
{
   Eina_List *l;
   Evas_Object *o;

   ed->message.func = func;
   ed->message.data = data;
   EINA_LIST_FOREACH(ed->subobjs, l, o)
     {
        Edje *edj2 = _edje_fetch(o);
        if (!edj2) continue;
        _edje_message_cb_set(edj2, func, data);
     }
}

/**
 * @brief Creates a new Edje_Message structure.
 *
 * This function allocates and initializes an Edje_Message. It first attempts
 * to reuse a message structure from the `_edje_msg_trash` cache. If the cache
 * is empty, it allocates a new structure.
 * The message count for the associated Edje object is incremented.
 *
 * @param ed The Edje object this message is associated with.
 * @param queue The queue this message belongs to (EDJE_QUEUE_APP or EDJE_QUEUE_SCRIPT).
 * @param type The type of the message.
 * @param id The ID of the message.
 * @return A pointer to the newly created Edje_Message, or NULL on allocation failure.
 */
Edje_Message *
_edje_message_new(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id)
{
   Edje_Message *em;

   em = _edje_msg_trash_pop();
   if (em) memset(em, 0, sizeof(Edje_Message));
   else em = calloc(1, sizeof(Edje_Message));
   if (!em) return NULL;
   em->edje = ed;
   em->edje->message.num++;
   em->queue = queue;
   em->type = type;
   em->id = id;
   return em;
}

/**
 * @brief Frees an Edje_Message structure and its associated payload.
 *
 * This function deallocates the payload (`em->msg`) of an Edje_Message,
 * depending on its type. After freeing the payload, the Edje_Message
 * structure itself is not freed directly but pushed onto the `_edje_msg_trash`
 * list for potential reuse.
 *
 * @param em The Edje_Message to free.
 */
void
_edje_message_free(Edje_Message *em)
{
   if (em->msg)
     {
        int i;

        switch (em->type)
          {
           case EDJE_MESSAGE_STRING:
           {
              Edje_Message_String *emsg;

              emsg = (Edje_Message_String *)em->msg;
              free(emsg->str);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_INT:
           {
              Edje_Message_Int *emsg;

              emsg = (Edje_Message_Int *)em->msg;
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_FLOAT:
           {
              Edje_Message_Float *emsg;

              emsg = (Edje_Message_Float *)em->msg;
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_INT_SET:
           {
              Edje_Message_Int_Set *emsg;

              emsg = (Edje_Message_Int_Set *)em->msg;
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_FLOAT_SET:
           {
              Edje_Message_Float_Set *emsg;

              emsg = (Edje_Message_Float_Set *)em->msg;
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_STRING_FLOAT:
           {
              Edje_Message_String_Float *emsg;

              emsg = (Edje_Message_String_Float *)em->msg;
              free(emsg->str);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_STRING_INT:
           {
              Edje_Message_String_Int *emsg;

              emsg = (Edje_Message_String_Int *)em->msg;
              free(emsg->str);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_STRING_FLOAT_SET:
           {
              Edje_Message_String_Float_Set *emsg;

              emsg = (Edje_Message_String_Float_Set *)em->msg;
              free(emsg->str);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_STRING_INT_SET:
           {
              Edje_Message_String_Int_Set *emsg;

              emsg = (Edje_Message_String_Int_Set *)em->msg;
              free(emsg->str);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_SIGNAL:
           {
              Edje_Message_Signal *emsg;

              emsg = (Edje_Message_Signal *)em->msg;
              if (emsg->sig) eina_stringshare_del(emsg->sig);
              if (emsg->src) eina_stringshare_del(emsg->src);
              _edje_signal_data_free(emsg->data);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_STRING_SET:
           {
              Edje_Message_String_Set *emsg;

              emsg = (Edje_Message_String_Set *)em->msg;
              for (i = 0; i < emsg->count; i++)
                free(emsg->str[i]);
              free(emsg);
           }
           break;

           case EDJE_MESSAGE_NONE:
           default:
             break;
          }
     }
   _edje_msg_trash_push(em);
}

/**
 * @brief Core function to create, populate, and queue an Edje message.
 *
 * This function is responsible for:
 * 1. Creating a new Edje_Message structure (or reusing one from trash).
 * 2. Setting up Ecore_Job or Ecore_Timer to ensure the message queue is processed.
 *    - If called from within an Ecore_Job (`_injob > 0`), it sets a timer
 *      to avoid issues with job processing.
 *    - Otherwise, it adds an Ecore_Job to process the queue.
 * 3. Deep-copying the message payload (`emsg`) based on its `type`. This is
 *    crucial because the original `emsg` might be a temporary or stack-allocated variable.
 * 4. Appending the newly created and populated message to the global message queue (`msgq`)
 *    and to the Edje object's specific message list.
 *
 * @param ed The Edje object to which the message is being sent.
 * @param queue The queue type (EDJE_QUEUE_SCRIPT or EDJE_QUEUE_APP).
 * @param type The type of the message (e.g., EDJE_MESSAGE_STRING, EDJE_MESSAGE_INT).
 * @param id An integer identifier for the message.
 * @param emsg A pointer to the raw message data. The structure of this data
 *             depends on the `type`. For example:
 *             - For `EDJE_MESSAGE_STRING`: `Edje_Message_String*`
 *             - For `EDJE_MESSAGE_INT`: `Edje_Message_Int*`
 *             - For `EDJE_MESSAGE_FLOAT_SET`: `Edje_Message_Float_Set*` containing an array of doubles.
 * @param prop A boolean indicating if this message is being propagated (e.g., from a parent to a child Edje object).
 */
static void
_edje_message_propagate_send(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id, void *emsg, Eina_Bool prop)
{
   /* FIXME: check all malloc & strdup fails and gracefully unroll and exit */
   Edje_Message *em;
   int i;
   unsigned char *msg = NULL;

   em = _edje_message_new(ed, queue, type, id);
   if (!em) return;
   em->propagated = prop;
   if (_job)
     {
        ecore_job_del(_job);
        _job = NULL;
     }
   if (_injob > 0)
     {
        if (!_job_loss_timer)
          _job_loss_timer = ecore_timer_add(0.000, _edje_job_loss_timer, NULL);
     }
   else
     {
        if (!_job)
          {
             _job = ecore_job_add(_edje_job, NULL);
          }
        if (_job_loss_timer)
          {
             ecore_timer_del(_job_loss_timer);
             _job_loss_timer = NULL;
          }
     }
   switch (em->type)
     {
      case EDJE_MESSAGE_NONE:
        break;

      case EDJE_MESSAGE_SIGNAL:
      {
         Edje_Message_Signal *emsg2, *emsg3;

         emsg2 = (Edje_Message_Signal *)emsg;
         emsg3 = calloc(1, sizeof(Edje_Message_Signal));
         if (emsg2->sig) emsg3->sig = eina_stringshare_add(emsg2->sig);
         if (emsg2->src) emsg3->src = eina_stringshare_add(emsg2->src);
         if (emsg2->data)
           {
              emsg3->data = emsg2->data;
              _edje_signal_data_ref(emsg3->data);
           }
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING:
      {
         Edje_Message_String *emsg2, *emsg3;

         emsg2 = (Edje_Message_String *)emsg;

         emsg3 = malloc(sizeof(Edje_Message_String));
         emsg3->str = strdup(emsg2->str);
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_INT:
      {
         Edje_Message_Int *emsg2, *emsg3;

         emsg2 = (Edje_Message_Int *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_Int));
         emsg3->val = emsg2->val;
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_FLOAT:
      {
         Edje_Message_Float *emsg2, *emsg3;

         emsg2 = (Edje_Message_Float *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_Float));
         emsg3->val = emsg2->val;
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING_SET:
      {
         Edje_Message_String_Set *emsg2, *emsg3;

         emsg2 = (Edje_Message_String_Set *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_String_Set) + ((emsg2->count - 1) * sizeof(char *)));
         emsg3->count = emsg2->count;
         for (i = 0; i < emsg3->count; i++)
           emsg3->str[i] = strdup(emsg2->str[i]);
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_INT_SET:
      {
         Edje_Message_Int_Set *emsg2, *emsg3;

         emsg2 = (Edje_Message_Int_Set *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_Int_Set) + ((emsg2->count - 1) * sizeof(int)));
         emsg3->count = emsg2->count;
         for (i = 0; i < emsg3->count; i++)
           emsg3->val[i] = emsg2->val[i];
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_FLOAT_SET:
      {
         Edje_Message_Float_Set *emsg2, *emsg3;

         emsg2 = (Edje_Message_Float_Set *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_Float_Set) + ((emsg2->count - 1) * sizeof(double)));
         emsg3->count = emsg2->count;
         for (i = 0; i < emsg3->count; i++)
           emsg3->val[i] = emsg2->val[i];
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING_INT:
      {
         Edje_Message_String_Int *emsg2, *emsg3;

         emsg2 = (Edje_Message_String_Int *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_String_Int));
         emsg3->str = strdup(emsg2->str);
         emsg3->val = emsg2->val;
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT:
      {
         Edje_Message_String_Float *emsg2, *emsg3;

         emsg2 = (Edje_Message_String_Float *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_String_Float));
         emsg3->str = strdup(emsg2->str);
         emsg3->val = emsg2->val;
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING_INT_SET:
      {
         Edje_Message_String_Int_Set *emsg2, *emsg3;

         emsg2 = (Edje_Message_String_Int_Set *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_String_Int_Set) + ((emsg2->count - 1) * sizeof(int)));
         emsg3->str = strdup(emsg2->str);
         emsg3->count = emsg2->count;
         for (i = 0; i < emsg3->count; i++)
           emsg3->val[i] = emsg2->val[i];
         msg = (unsigned char *)emsg3;
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT_SET:
      {
         Edje_Message_String_Float_Set *emsg2, *emsg3;

         emsg2 = (Edje_Message_String_Float_Set *)emsg;
         emsg3 = malloc(sizeof(Edje_Message_String_Float_Set) + ((emsg2->count - 1) * sizeof(double)));
         emsg3->str = strdup(emsg2->str);
         emsg3->count = emsg2->count;
         for (i = 0; i < emsg3->count; i++)
           emsg3->val[i] = emsg2->val[i];
         msg = (unsigned char *)emsg3;
      }
      break;

      default:
        break;
     }

   em->msg = msg;
   msgq = eina_inlist_append(msgq, &(em->inlist_main));
   em->edje->messages = eina_inlist_append(em->edje->messages, &(em->inlist_edje));
}

/**
 * @brief Utility function to send a message to an Edje object without propagation.
 *
 * This is a wrapper around `_edje_message_propagate_send` that explicitly
 * sets the propagation flag to `EINA_FALSE`.
 *
 * @param ed The Edje object to send the message to.
 * @param queue The queue type for the message.
 * @param type The type of the message.
 * @param id The ID of the message.
 * @param emsg A pointer to the message data.
 */
void
_edje_util_message_send(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id, void *emsg)
{
   _edje_message_propagate_send(ed, queue, type, id, emsg, EINA_FALSE);
}

/**
 * @brief Pushes message parameters onto the Embryo script stack.
 *
 * This function prepares parameters for an Embryo script's "message" handler
 * based on the content of an Edje_Message.
 * The first two parameters pushed are always:
 * 1. The message type (`em->type`) as an Embryo_Cell.
 * 2. The message ID (`em->id`) as an Embryo_Cell.
 * Subsequent parameters depend on the message type and its payload.
 *
 * For example:
 * - `EDJE_MESSAGE_STRING`: Pushes the string.
 * - `EDJE_MESSAGE_INT`: Pushes the integer value.
 * - `EDJE_MESSAGE_FLOAT_SET`: Pushes each float value in the set.
 *   The `val` array in `Edje_Message_Float_Set` would be pushed one by one.
 *   If `((Edje_Message_Float_Set *)em->msg)->val` is `{1.0, 2.5, 3.0}`,
 *   three float parameters will be pushed onto the Embryo stack.
 *
 * @param em The Edje_Message whose parameters are to be pushed.
 */
void
_edje_message_parameters_push(Edje_Message *em)
{
   int i;

   /* these params ALWAYS go on */
   /* first param is the message type - always */
   embryo_parameter_cell_push(em->edje->collection->script,
                              (Embryo_Cell)em->type);
   /* 2nd param is the integer of the event id - always there */
   embryo_parameter_cell_push(em->edje->collection->script,
                              (Embryo_Cell)em->id);
   /* the rest is varags of whatever is in the msg */
   switch (em->type)
     {
      case EDJE_MESSAGE_NONE:
        break;

      case EDJE_MESSAGE_STRING:
        embryo_parameter_string_push(em->edje->collection->script,
                                     ((Edje_Message_String *)em->msg)->str);
        break;

      case EDJE_MESSAGE_INT:
      {
         Embryo_Cell v;

         v = (Embryo_Cell)((Edje_Message_Int *)em->msg)->val;
         embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
      }
      break;

      case EDJE_MESSAGE_FLOAT:
      {
         Embryo_Cell v;
         float fv;

         fv = ((Edje_Message_Float *)em->msg)->val;
         v = EMBRYO_FLOAT_TO_CELL(fv);
         embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
      }
      break;

      case EDJE_MESSAGE_STRING_SET:
        for (i = 0; i < ((Edje_Message_String_Set *)em->msg)->count; i++)
          embryo_parameter_string_push(em->edje->collection->script,
                                       ((Edje_Message_String_Set *)em->msg)->str[i]);
        break;

      case EDJE_MESSAGE_INT_SET:
        for (i = 0; i < ((Edje_Message_Int_Set *)em->msg)->count; i++)
          {
             Embryo_Cell v;

             v = (Embryo_Cell)((Edje_Message_Int_Set *)em->msg)->val[i];
             embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
          }
        break;

      case EDJE_MESSAGE_FLOAT_SET:
        for (i = 0; i < ((Edje_Message_Float_Set *)em->msg)->count; i++)
          {
             Embryo_Cell v;
             float fv;

             fv = ((Edje_Message_Float_Set *)em->msg)->val[i];
             v = EMBRYO_FLOAT_TO_CELL(fv);
             embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
          }
        break;

      case EDJE_MESSAGE_STRING_INT:
        embryo_parameter_string_push(em->edje->collection->script,
                                     ((Edje_Message_String_Int *)em->msg)->str);
        {
           Embryo_Cell v;

           v = (Embryo_Cell)((Edje_Message_String_Int *)em->msg)->val;
           embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
        }
        break;

      case EDJE_MESSAGE_STRING_FLOAT:
        embryo_parameter_string_push(em->edje->collection->script,
                                     ((Edje_Message_String_Float *)em->msg)->str);
        {
           Embryo_Cell v;
           float fv;

           fv = ((Edje_Message_String_Float *)em->msg)->val;
           v = EMBRYO_FLOAT_TO_CELL(fv);
           embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
        }
        break;

      case EDJE_MESSAGE_STRING_INT_SET:
        embryo_parameter_string_push(em->edje->collection->script,
                                     ((Edje_Message_String_Int_Set *)em->msg)->str);
        for (i = 0; i < ((Edje_Message_String_Int_Set *)em->msg)->count; i++)
          {
             Embryo_Cell v;

             v = (Embryo_Cell)((Edje_Message_String_Int_Set *)em->msg)->val[i];
             embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
          }
        break;

      case EDJE_MESSAGE_STRING_FLOAT_SET:
        embryo_parameter_string_push(em->edje->collection->script,
                                     ((Edje_Message_String_Float_Set *)em->msg)->str);
        for (i = 0; i < ((Edje_Message_String_Float_Set *)em->msg)->count; i++)
          {
             Embryo_Cell v;
             float fv;

             fv = ((Edje_Message_String_Float_Set *)em->msg)->val[i];
             v = EMBRYO_FLOAT_TO_CELL(fv);
             embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
          }
        break;

      default:
        break;
     }
}

/**
 * @brief Processes a single Edje_Message.
 *
 * This function determines how to handle an incoming Edje_Message based on
 * its type and queue.
 * - If the message type is `EDJE_MESSAGE_SIGNAL`, it's handled by `_edje_emit_handle`.
 * - If the message queue is `EDJE_QUEUE_APP`, the registered C callback
 *   (`em->edje->message.func`) is invoked.
 * - If the message queue is `EDJE_QUEUE_SCRIPT`:
 *   - If Lua scripting is active (`em->edje->L`), `_edje_lua_script_only_message` is called.
 *   - Otherwise, it attempts to find and execute an Embryo script function named "message".
 *     It sets up the Embryo VM, pushes parameters using `_edje_message_parameters_push`,
 *     runs the script, and handles potential errors.
 *
 * @param em The Edje_Message to process.
 */
void
_edje_message_process(Edje_Message *em)
{
   Embryo_Function fn;
   void *pdata;
   int ret;

   /* signals are only handled one way */
   if (em->type == EDJE_MESSAGE_SIGNAL)
     {
        _edje_emit_handle(em->edje,
                          ((Edje_Message_Signal *)em->msg)->sig,
                          ((Edje_Message_Signal *)em->msg)->src,
                          ((Edje_Message_Signal *)em->msg)->data,
                          em->propagated);
        return;
     }
   /* if this has been queued up for the app then just call the callback */
   if (em->queue == EDJE_QUEUE_APP)
     {
        if (em->edje->message.func)
          em->edje->message.func(em->edje->message.data, em->edje->obj,
                                 em->type, em->id, em->msg);
        return;
     }
   /* now this message is destined for the script message handler fn */
   if (!(em->edje->collection)) return;
   if (em->edje->L)
     {
        _edje_lua_script_only_message(em->edje, em);
        return;
     }
   fn = embryo_program_function_find(em->edje->collection->script, "message");
   if (fn == EMBRYO_FUNCTION_NONE) return;
   /* reset the engine */
   _edje_embryo_script_reset(em->edje);

   _edje_message_parameters_push(em);

   embryo_program_vm_push(em->edje->collection->script);
   _edje_embryo_globals_init(em->edje);
   pdata = embryo_program_data_get(em->edje->collection->script);
   embryo_program_data_set(em->edje->collection->script, em->edje);
   embryo_program_max_cycle_run_set(em->edje->collection->script, 5000000);
   ret = embryo_program_run(em->edje->collection->script, fn);
   if (ret == EMBRYO_PROGRAM_FAIL)
     {
        ERR("ERROR with embryo script. "
            "OBJECT NAME: '%s', "
            "OBJECT FILE: '%s', "
            "ENTRY POINT: '%s', "
            "ERROR: '%s'",
            em->edje->collection->part,
            em->edje->file->path,
            "message",
            embryo_error_string_get(embryo_program_error_get(em->edje->collection->script)));
     }
   else if (ret == EMBRYO_PROGRAM_TOOLONG)
     {
        ERR("ERROR with embryo script. "
            "OBJECT NAME: '%s', "
            "OBJECT FILE: '%s', "
            "ENTRY POINT: '%s', "
            "ERROR: 'Script exceeded maximum allowed cycle count of %i'",
            em->edje->collection->part,
            em->edje->file->path,
            "message",
            embryo_program_max_cycle_run_get(em->edje->collection->script));
     }

   embryo_program_data_set(em->edje->collection->script, pdata);
   embryo_program_vm_pop(em->edje->collection->script);
}

/**
 * @brief Processes all messages in the global Edje message queue.
 *
 * This function iterates through the main message queue (`msgq`), moving messages
 * to a temporary queue (`tmp_msgq`) for processing. This is done to handle
 * cases where processing a message might add new messages to the queue.
 * It processes messages in batches (up to 8 iterations of filling and draining
 * `tmp_msgq`) to prevent excessively long processing loops within a single call.
 * If messages still remain after these iterations (a "self-feeding message loop"),
 * it schedules a 0-delay timer to re-trigger processing, allowing other events
 * to be handled.
 *
 * During processing, each message is removed from its Edje object's local list
 * and the global temporary queue, then passed to `_edje_message_process`.
 * After processing, the message is freed. It also handles Edje object deletion
 * if `delete_me` is set and no messages are being processed for it.
 *
 * The `tmp_msgq_restart` flag is used to signal if re-evaluation of the queue
 * is needed due to re-entrant calls.
 */
void
_edje_message_queue_process(void)
{
   int i;
   Edje_Message *em;

   if (!msgq) return;

   /* allow the message queue to feed itself up to 8 times before forcing */
   /* us to go back to normal processing and let a 0 timeout deal with it */
   for (i = 0; (i < 8) && (msgq); i++)
     {
        /* a temporary message queue */
        while (msgq)
          {
             Eina_Inlist *l = msgq;

             em = INLIST_CONTAINER(Edje_Message, l, inlist_main);
             msgq = eina_inlist_remove(msgq, &(em->inlist_main));
             tmp_msgq = eina_inlist_append(tmp_msgq, &(em->inlist_main));
             em->in_tmp_msgq = EINA_TRUE;
          }

        tmp_msgq_processing++;
        while (tmp_msgq)
          {
             Eina_Inlist *l = tmp_msgq;
             Edje *ed;

             em = INLIST_CONTAINER(Edje_Message, l, inlist_main);
             ed = em->edje;
             tmp_msgq = eina_inlist_remove(tmp_msgq, &(em->inlist_main));
             em->edje->messages = eina_inlist_remove(em->edje->messages, &(em->inlist_edje));
             em->edje->message.num--;
             if (!ed->delete_me)
               {
                  ed->processing_messages++;
                  _edje_message_process(em);
                  _edje_message_free(em);
                  ed->processing_messages--;
               }
             else
               _edje_message_free(em);
             if (ed->processing_messages == 0)
               {
                  if (ed->delete_me) _edje_del(ed);
               }
          }
        tmp_msgq_processing--;
        if (tmp_msgq_processing == 0)
          tmp_msgq_restart = 0;
        else
          tmp_msgq_restart = 1;
     }

   /* if the message queue filled again set a timer to expire in 0.0 sec */
   /* to get the idle enterer to be run again */
   if (msgq)
     {
        static int self_feed_debug = -1;

        if (self_feed_debug == -1)
          {
             const char *s = getenv("EDJE_SELF_FEED_DEBUG");
             if (s) self_feed_debug = atoi(s);
             else self_feed_debug = 0;
          }
        if (self_feed_debug)
          {
             WRN("Edje is in a self-feeding message loop (> 8 loops needed)");
          }
        ecore_timer_add(0.0, _edje_dummy_timer, NULL);
     }
}

/**
 * @brief Clears all messages from both the main and temporary Edje message queues.
 *
 * This function iterates through `msgq` and `tmp_msgq`, removing each message
 * from its associated Edje object's message list, decrementing the message count
 * for that object, and then freeing the message structure using `_edje_message_free`.
 * This is typically used during shutdown or when a major state reset is required.
 */
void
_edje_message_queue_clear(void)
{
   Edje_Message *em;

   while (msgq)
     {
        Eina_Inlist *l = msgq;
        em = INLIST_CONTAINER(Edje_Message, l, inlist_main);
        msgq = eina_inlist_remove(msgq, &(em->inlist_main));
        em->edje->message.num--;
        em->edje->messages = eina_inlist_remove(em->edje->messages, &(em->inlist_edje));
        _edje_message_free(em);
     }
   while (tmp_msgq)
     {
        Eina_Inlist *l = tmp_msgq;
        em = INLIST_CONTAINER(Edje_Message, l, inlist_main);
        tmp_msgq = eina_inlist_remove(tmp_msgq, &(em->inlist_main));
        em->edje->message.num--;
        em->edje->messages = eina_inlist_remove(em->edje->messages, &(em->inlist_edje));
        _edje_message_free(em);
     }
}

/**
 * @brief Deletes all messages associated with a specific Edje object from the queues.
 *
 * This function iterates through the messages currently linked to the given
 * Edje object (`ed->messages`). For each message found:
 * 1. It decrements the message count on the Edje object.
 * 2. It removes the message from either the main queue (`msgq`) or the
 *    temporary queue (`tmp_msgq`), depending on where it currently resides
 *    (indicated by `em->in_tmp_msgq`).
 * 3. It removes the message from the Edje object's own list of messages.
 * 4. It frees the message using `_edje_message_free`.
 * The process stops if the Edje object's message count drops to zero.
 *
 * This is typically called when an Edje object is being deleted to ensure
 * no pending messages for it remain in the system.
 *
 * @param ed The Edje object whose messages are to be deleted.
 */
void
_edje_message_del(Edje *ed)
{
   Eina_Inlist *l, *ln;
   Edje_Message *em;

   if (ed->message.num <= 0) return;
   // delete any messages on the main or tmp queue for this edje object
   for (l = ed->messages; l; l = ln)
     {
        ln = l->next;
        em = INLIST_CONTAINER(Edje_Message, l, inlist_edje);
        em->edje->message.num--;
        if (em->in_tmp_msgq)
          tmp_msgq = eina_inlist_remove(tmp_msgq, &(em->inlist_main));
        else
          msgq = eina_inlist_remove(msgq, &(em->inlist_main));
        em->edje->messages = eina_inlist_remove(em->edje->messages, &(em->inlist_edje));
        _edje_message_free(em);
        if (ed->message.num <= 0) return;
     }
}

/* Legacy EAPI */

EAPI void
edje_object_message_send(Eo *obj, Edje_Message_Type type, int id, void *msg)
{
   _edje_object_message_propagate_send(obj, type, id, msg, EINA_FALSE);
}

EAPI void
edje_message_signal_process(void)
{
   _edje_message_queue_process();
}

EAPI void
edje_object_message_handler_set(Eo *obj, Edje_Message_Handler_Cb func, void *data)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return;
   _edje_message_cb_set(ed, func, data);
}
