#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_CLOSER_PROTECTED 1

#include <Ecore.h>
#include "ecore_private.h"

/**
 * @brief Private data structure for Efl_Io_Buffered_Stream.
 */
typedef struct
{
   Eo *inner_io; /**< The underlying I/O object this buffered stream wraps. */
   Eo *incoming; /**< Efl_Io_Queue used to buffer data read from inner_io. */
   Eo *outgoing; /**< Efl_Io_Queue used to buffer data to be written to inner_io. */
   Eo *sender;   /**< Efl_Io_Copier that moves data from outgoing queue to inner_io. */
   Eo *receiver; /**< Efl_Io_Copier that moves data from inner_io to incoming queue. */
   Eina_Bool is_closing; /**< Flag to indicate if the stream is in the process of closing. Prevents re-entry. */
   Eina_Bool closed;     /**< Flag to indicate if the stream has been closed. */
   Eina_Bool eos;        /**< Flag to indicate if End-Of-Stream has been reached for reading. */
   Eina_Bool can_read;   /**< Flag to indicate if the stream is currently readable. */
   Eina_Bool can_write;  /**< Flag to indicate if the stream is currently writable. */
   Eina_Bool is_closer;  /**< Flag to indicate if the inner_io object implements Efl_Io_Closer. */
   Eina_Bool is_finished; /**< Flag to indicate if both read and write operations are complete. */
} Efl_Io_Buffered_Stream_Data;

#define MY_CLASS EFL_IO_BUFFERED_STREAM_CLASS

/**
 * @brief Event callback triggered when an error occurs in one of the internal copiers or the inner I/O.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The error event, with event->info containing an Eina_Error pointer.
 */
static void
_efl_io_buffered_stream_error(void *data, const Efl_Event *event)
{
   Eo *o = data;
   Eina_Error *perr = event->info;
   DBG("%p %s error: %s", o, efl_name_get(event->object), eina_error_msg_get(*perr));
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_ERROR, event->info);
}

/**
 * @brief Event callback triggered when an internal copier makes progress.
 *
 * This function forwards the progress event to the Efl_Io_Buffered_Stream object.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The progress event (unused).
 */
static void
_efl_io_buffered_stream_copier_progress(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_PROGRESS, NULL);
}

/**
 * @brief Event callback for 'can_read,changed' on the incoming Efl_Io_Queue.
 *
 * Updates the 'can_read' status of the Efl_Io_Buffered_Stream.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'can_read,changed' event from the incoming queue.
 */
static void
_efl_io_buffered_stream_incoming_can_read_changed(void *data, const Efl_Event *event)
{
   Eo *o = data;
   if (efl_io_closer_closed_get(o)) return; /* already closed (or closing) */
   efl_io_reader_can_read_set(o, efl_io_reader_can_read_get(event->object));
}

/**
 * @brief Event callback for 'slice,changed' on the incoming Efl_Io_Queue.
 *
 * Forwards the 'slice,changed' event, indicating new data is available in the read buffer.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'slice,changed' event from the incoming queue (unused).
 */
static void
_efl_io_buffered_stream_incoming_slice_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_SLICE_CHANGED, NULL);
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_io_buffered_stream_incoming_cbs,
                           { EFL_IO_READER_EVENT_CAN_READ_CHANGED, _efl_io_buffered_stream_incoming_can_read_changed },
                           { EFL_IO_QUEUE_EVENT_SLICE_CHANGED, _efl_io_buffered_stream_incoming_slice_changed });

/**
 * @brief Event callback for a 'line' event from the receiver (Efl_Io_Copier).
 *
 * Forwards the 'line' event, which contains the detected line as event->info.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'line' event from the receiver.
 */
static void
_efl_io_buffered_stream_receiver_line(void *data, const Efl_Event *event)
{
   Eo *o = data;
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_LINE, event->info);
}

/**
 * @brief Event callback for 'done' event from the receiver (Efl_Io_Copier).
 *
 * This indicates that the receiver has finished copying data from the inner_io
 * (e.g., inner_io reached EOS). Sets the EOS flag on the buffered stream.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'done' event from the receiver (unused).
 */
static void
_efl_io_buffered_stream_receiver_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   if (efl_io_closer_closed_get(o)) return; /* already closed (or closing) */
   efl_io_reader_eos_set(o, EINA_TRUE);
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_io_buffered_stream_receiver_cbs,
                           { EFL_IO_COPIER_EVENT_PROGRESS, _efl_io_buffered_stream_copier_progress },
                           { EFL_IO_COPIER_EVENT_DONE, _efl_io_buffered_stream_receiver_done },
                           { EFL_IO_COPIER_EVENT_LINE, _efl_io_buffered_stream_receiver_line },
                           { EFL_IO_COPIER_EVENT_ERROR, _efl_io_buffered_stream_error });

/**
 * @brief Event callback for 'can_write,changed' on the outgoing Efl_Io_Queue.
 *
 * Updates the 'can_write' status of the Efl_Io_Buffered_Stream.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'can_write,changed' event from the outgoing queue.
 */
static void
_efl_io_buffered_stream_outgoing_can_write_changed(void *data, const Efl_Event *event)
{
   Eo *o = data;
   if (efl_io_closer_closed_get(o)) return; /* already closed (or closing) */
   efl_io_writer_can_write_set(o, efl_io_writer_can_write_get(event->object));
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_io_buffered_stream_outgoing_cbs,
                           { EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, _efl_io_buffered_stream_outgoing_can_write_changed });

/**
 * @brief Event callback for 'done' event from the sender (Efl_Io_Copier).
 *
 * This indicates that the sender has finished copying all data from the outgoing
 * queue to the inner_io. Triggers 'write_finished' and potentially 'finished' events.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The 'done' event from the sender (unused).
 */
static void
_efl_io_buffered_stream_sender_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Buffered_Stream_Data *pd = efl_data_scope_get(o, MY_CLASS);
   size_t pending = pd->receiver ? efl_io_copier_pending_size_get(pd->receiver) : 0;

   efl_ref(o);
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_PROGRESS, NULL);
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_WRITE_FINISHED, NULL);
   if ((!pd->receiver) || efl_io_copier_done_get(pd->receiver))
     {
        if (!pd->is_finished)
          {
             pd->is_finished = EINA_TRUE;
             efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_FINISHED, NULL);
          }
     }
   else
     DBG("%p sender done, waiting for receiver to process %zd to call it 'finished'", o, pending);
   efl_unref(o);
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_io_buffered_stream_sender_cbs,
                           { EFL_IO_COPIER_EVENT_PROGRESS, _efl_io_buffered_stream_copier_progress },
                           { EFL_IO_COPIER_EVENT_DONE, _efl_io_buffered_stream_sender_done },
                           { EFL_IO_COPIER_EVENT_ERROR, _efl_io_buffered_stream_error });

/**
 * @brief Event callback for EFL_EVENT_DEL on the inner_io object.
 *
 * This is called if the inner_io object is deleted externally.
 * It nullifies the reference to inner_io in the buffered stream's data.
 *
 * @param data The Efl_Io_Buffered_Stream object.
 * @param event The EFL_EVENT_DEL event, with event->object being the deleted inner_io.
 */
static void
_efl_io_buffered_stream_inner_io_del(void *data, const Efl_Event *event)
{
   Eo *o = data;
   Efl_Io_Buffered_Stream_Data *pd = efl_data_scope_get(o, MY_CLASS);
   DBG("%p the inner I/O %p was deleted", o, event->object);
   if (pd->inner_io == event->object)
     pd->inner_io = NULL;
}

EFL_CALLBACKS_ARRAY_DEFINE(_efl_io_buffered_stream_inner_io_cbs,
                           { EFL_EVENT_DEL, _efl_io_buffered_stream_inner_io_del });


EOLIAN static Efl_Object *
_efl_io_buffered_stream_efl_object_finalize(Eo *o, Efl_Io_Buffered_Stream_Data *pd)
{
   // Ensure inner_io is set before finalization.
   if (!pd->inner_io)
     {
        ERR("no valid I/O was set with efl_io_buffered_stream_inner_io_set()!");
        return NULL;
     }

   return efl_finalize(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_io_buffered_stream_efl_object_invalidate(Eo *o, Efl_Io_Buffered_Stream_Data *pd)
{
   // Ensure the stream is closed during invalidation.
   if (!efl_io_closer_closed_get(o))
     efl_io_closer_close(o);

   // Clean up resources related to inner_io.
   if (pd->inner_io)
     {
        efl_event_callback_array_del(pd->inner_io, _efl_io_buffered_stream_inner_io_cbs(), o);
        if (efl_parent_get(pd->inner_io) == o)
          efl_parent_set(pd->inner_io, NULL);
        else
          efl_unref(pd->inner_io); /* do not del, just take our ref */
        pd->inner_io = NULL;
     }

   pd->incoming = NULL;
   pd->outgoing = NULL;
   pd->sender = NULL;
   pd->receiver = NULL;

   // Ensure the 'finished' event is emitted if it hasn't been already.
   if (!pd->is_finished)
     {
        pd->is_finished = EINA_TRUE;
        efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_FINISHED, NULL);
     }

   efl_invalidate(efl_super(o, MY_CLASS));
}

EOLIAN static Eina_Error
_efl_io_buffered_stream_efl_io_closer_close(Eo *o, Efl_Io_Buffered_Stream_Data *pd)
{
   Eina_Error err = 0;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->closed, EINVAL); // Already closed
   if (pd->is_closing) return 0; // Already in the process of closing
   pd->is_closing = EINA_TRUE;

   // Mark outgoing queue as EOS and attempt to flush remaining data.
   if (pd->outgoing)
     {
        efl_io_queue_eos_mark(pd->outgoing);
        efl_io_copier_flush(pd->sender, EINA_FALSE, EINA_TRUE); // Non-blocking flush
     }

   /* line delimiters may be holding a last chunk of data, flush receiver too */
   if (pd->receiver) efl_io_copier_flush(pd->receiver, EINA_FALSE, EINA_TRUE); // Non-blocking flush

   // Update stream state to reflect closure.
   efl_io_writer_can_write_set(o, EINA_FALSE);
   efl_io_reader_can_read_set(o, EINA_FALSE);
   efl_io_reader_eos_set(o, EINA_TRUE);

   pd->closed = EINA_TRUE;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);

   // Close internal copiers if they are not already closed.
   if (pd->sender && (!efl_io_closer_closed_get(pd->sender)))
     efl_io_closer_close(pd->sender);

   if (pd->receiver && (!efl_io_closer_closed_get(pd->receiver)))
     efl_io_closer_close(pd->receiver);

   return err;
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_closer_closed_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // The stream is considered closed if explicitly closed or if the inner_io is closed.
   return pd->closed || efl_io_closer_closed_get(pd->inner_io);
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_closer_close_on_exec_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Delegate close_on_exec to the inner_io object.
   return efl_io_closer_close_on_exec_get(pd->inner_io);
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_closer_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, Eina_Bool value)
{
   // Delegate close_on_exec setting to the inner_io object.
   return efl_io_closer_close_on_exec_set(pd->inner_io, value);
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Delegate close_on_invalidate to the inner_io object.
   return efl_io_closer_close_on_invalidate_get(pd->inner_io);
}

EOLIAN static void
_efl_io_buffered_stream_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, Eina_Bool value)
{
   // Delegate close_on_invalidate setting to the inner_io object.
   efl_io_closer_close_on_invalidate_set(pd->inner_io, value);
}

EOLIAN static Eina_Error
_efl_io_buffered_stream_efl_io_reader_read(Eo *o, Efl_Io_Buffered_Stream_Data *pd, Eina_Rw_Slice *rw_slice)
{
   Eina_Error err;

   // Reading is done from the incoming queue, which buffers data from inner_io.
   if (!pd->incoming)
     {
        WRN("%p reading from inner_io %p (%s) that doesn't implement Efl.Io.Reader",
            o, pd->inner_io, efl_class_name_get(efl_class_get(pd->inner_io)));
        return EINVAL;
     }

   err = efl_io_reader_read(pd->incoming, rw_slice);
   if (err && (err != EAGAIN)) // Report errors other than EAGAIN.
     efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_ERROR, &err);
   return err;
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_reader_can_read_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Return the cached can_read status.
   return pd->can_read;
}

EOLIAN static void
_efl_io_buffered_stream_efl_io_reader_can_read_set(Eo *o, Efl_Io_Buffered_Stream_Data *pd EINA_UNUSED, Eina_Bool can_read)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o)); // Do not change if closed.
   if (pd->can_read == can_read) return; // No change.
   pd->can_read = can_read;
   efl_event_callback_call(o, EFL_IO_READER_EVENT_CAN_READ_CHANGED, &can_read);
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_reader_eos_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Return the cached EOS status.
   return pd->eos;
}

EOLIAN static void
_efl_io_buffered_stream_efl_io_reader_eos_set(Eo *o, Efl_Io_Buffered_Stream_Data *pd, Eina_Bool is_eos)
{
   size_t pending = pd->sender ? efl_io_copier_pending_size_get(pd->sender) : 0;

   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o)); // Do not change if closed.
   if (pd->eos == is_eos) return; // No change.
   pd->eos = is_eos;
   if (!is_eos) return; // Only act if EOS is being set to true.

   efl_ref(o); // Protect against deletion during event callbacks.
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_PROGRESS, NULL);
   efl_event_callback_call(o, EFL_IO_READER_EVENT_EOS, NULL);
   efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_READ_FINISHED, NULL);

   // If the sender side is also done (or not used), then the stream is fully finished.
   if ((!pd->sender) || efl_io_copier_done_get(pd->sender))
     {
        if (!pd->is_finished)
          {
             pd->is_finished = EINA_TRUE;
             efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_FINISHED, NULL);
          }
     }
   else
     DBG("%p eos, waiting for sender process %zd to call 'finished'", o, pending);
   efl_unref(o);
}

EOLIAN static Eina_Error
_efl_io_buffered_stream_efl_io_writer_write(Eo *o, Efl_Io_Buffered_Stream_Data *pd, Eina_Slice *slice, Eina_Slice *remaining)
{
   Eina_Error err;

   // Writing is done to the outgoing queue, which is then copied to inner_io by the sender.
   if (!pd->outgoing)
     {
        WRN("%p writing to inner_io %p (%s) that doesn't implement Efl.Io.Writer",
            o, pd->inner_io, efl_class_name_get(efl_class_get(pd->inner_io)));
        return EINVAL;
     }

   err = efl_io_writer_write(pd->outgoing, slice, remaining);
   if (err && (err != EAGAIN)) // Report errors other than EAGAIN.
     efl_event_callback_call(o, EFL_IO_BUFFERED_STREAM_EVENT_ERROR, &err);
   return err;
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_efl_io_writer_can_write_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Return the cached can_write status.
   return pd->can_write;
}

EOLIAN static void
_efl_io_buffered_stream_efl_io_writer_can_write_set(Eo *o, Efl_Io_Buffered_Stream_Data *pd EINA_UNUSED, Eina_Bool can_write)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_io_closer_closed_get(o)); // Do not change if closed.
   if (pd->can_write == can_write) return; // No change.
   pd->can_write = can_write;
   efl_event_callback_call(o, EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, &can_write);
}

EOLIAN static void
_efl_io_buffered_stream_inner_io_set(Eo *o, Efl_Io_Buffered_Stream_Data *pd, Efl_Object *io)
{
   Eina_Bool is_reader, is_writer;

   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o)); // Cannot set after finalization.
   EINA_SAFETY_ON_NULL_RETURN(io); // Inner I/O object must be valid.
   EINA_SAFETY_ON_TRUE_RETURN(pd->inner_io != NULL); // Inner I/O already set.

   pd->is_closer = efl_isa(io, EFL_IO_CLOSER_INTERFACE);
   is_reader = efl_isa(io, EFL_IO_READER_INTERFACE);
   is_writer = efl_isa(io, EFL_IO_WRITER_INTERFACE);

   // The inner_io object must be at least a reader or a writer.
   EINA_SAFETY_ON_TRUE_RETURN((!is_reader) && (!is_writer));

   pd->inner_io = efl_ref(io); // Take a reference.
   efl_event_callback_array_add(io, _efl_io_buffered_stream_inner_io_cbs(), o); // Listen for DEL event.

   /* Setup for reading: inner_io -> receiver (copier) -> incoming (queue) */
   if (is_reader)
     {
        DBG("%p inner_io=%p (%s) is Efl.Io.Reader", o, io, efl_class_name_get(efl_class_get(io)));
        pd->incoming = efl_add(EFL_IO_QUEUE_CLASS, o,
                               efl_name_set(efl_added, "incoming"),
                               efl_event_callback_array_add(efl_added, _efl_io_buffered_stream_incoming_cbs(), o));
        EINA_SAFETY_ON_NULL_RETURN(pd->incoming); // Failed to create incoming queue.

        pd->receiver = efl_add(EFL_IO_COPIER_CLASS, o,
                               efl_name_set(efl_added, "receiver"),
                               efl_io_copier_buffer_limit_set(efl_added, 4096), // Default buffer limit.
                               efl_io_copier_source_set(efl_added, io),
                               efl_io_copier_destination_set(efl_added, pd->incoming),
                               efl_io_closer_close_on_invalidate_set(efl_added, efl_io_closer_close_on_invalidate_get(io)),
                               efl_event_callback_array_add(efl_added, _efl_io_buffered_stream_receiver_cbs(), o));
        EINA_SAFETY_ON_NULL_RETURN(pd->receiver); // Failed to create receiver copier.
     }
   else // If not a reader, mark EOS immediately.
     {
        DBG("%p inner_io=%p (%s) is not Efl.Io.Reader", o, io, efl_class_name_get(efl_class_get(io)));
        efl_io_reader_eos_set(o, EINA_TRUE);
     }


   /* Setup for writing: outgoing (queue) -> sender (copier) -> inner_io */
   if (is_writer)
     {
        DBG("%p inner_io=%p (%s) is Efl.Io.Writer", o, io, efl_class_name_get(efl_class_get(io)));
        pd->outgoing = efl_add(EFL_IO_QUEUE_CLASS, o,
                               efl_name_set(efl_added, "outgoing"),
                               efl_event_callback_array_add(efl_added, _efl_io_buffered_stream_outgoing_cbs(), o));
        EINA_SAFETY_ON_NULL_RETURN(pd->outgoing); // Failed to create outgoing queue.

        pd->sender = efl_add(EFL_IO_COPIER_CLASS, o,
                             efl_name_set(efl_added, "sender"),
                             efl_io_copier_buffer_limit_set(efl_added, 4096), // Default buffer limit.
                             efl_io_copier_source_set(efl_added, pd->outgoing),
                             efl_io_copier_destination_set(efl_added, io),
                             efl_io_closer_close_on_invalidate_set(efl_added, efl_io_closer_close_on_invalidate_get(io)),
                             efl_event_callback_array_add(efl_added, _efl_io_buffered_stream_sender_cbs(), o));
        EINA_SAFETY_ON_NULL_RETURN(pd->sender); // Failed to create sender copier.
     }
   else // If not a writer, nothing to set up for writing.
     DBG("%p inner_io=%p (%s) is not Efl.Io.Writer", o, io, efl_class_name_get(efl_class_get(io)));
}

EOLIAN static Efl_Object *
_efl_io_buffered_stream_inner_io_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Return the wrapped inner_io object.
   return pd->inner_io;
}

EOLIAN static void
_efl_io_buffered_stream_max_queue_size_input_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, size_t max_queue_size_input)
{
   // Set the limit on the incoming (read) buffer queue.
   if (!pd->incoming) // No incoming queue if inner_io is not a reader.
     {
        DBG("%p inner_io=%p (%s) is not Efl.Io.Reader, limit=%zu ignored", o, pd->inner_io, efl_class_name_get(efl_class_get(pd->inner_io)), max_queue_size_input);
        return;
     }
   efl_io_queue_limit_set(pd->incoming, max_queue_size_input);
}

EOLIAN static size_t
_efl_io_buffered_stream_max_queue_size_input_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the limit of the incoming (read) buffer queue.
   if (!pd->incoming) return 0;
   return efl_io_queue_limit_get(pd->incoming);
}

EOLIAN static void
_efl_io_buffered_stream_max_queue_size_output_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, size_t max_queue_size_output)
{
   // Set the limit on the outgoing (write) buffer queue.
   if (!pd->outgoing) // No outgoing queue if inner_io is not a writer.
     {
        DBG("%p inner_io=%p (%s) is not Efl.Io.Writer, limit=%zu ignored", o, pd->inner_io, efl_class_name_get(efl_class_get(pd->inner_io)), max_queue_size_output);
        return;
     }
   efl_io_queue_limit_set(pd->outgoing, max_queue_size_output);
}

EOLIAN static size_t
_efl_io_buffered_stream_max_queue_size_output_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the limit of the outgoing (write) buffer queue.
   if (!pd->outgoing) return 0;
   return efl_io_queue_limit_get(pd->outgoing);
}

EOLIAN static void
_efl_io_buffered_stream_line_delimiter_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, Eina_Slice slice)
{
   // Set the line delimiter for the receiver copier (used for line-based reading).
   if (!pd->receiver) // No receiver if inner_io is not a reader.
     {
        DBG("%p inner_io=%p (%s) is not Efl.Io.Reader, slice=" EINA_SLICE_FMT " ignored", o, pd->inner_io, efl_class_name_get(efl_class_get(pd->inner_io)), EINA_SLICE_PRINT(slice));
        return;
     }
   efl_io_copier_line_delimiter_set(pd->receiver, slice);
}

EOLIAN static Eina_Slice
_efl_io_buffered_stream_line_delimiter_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the line delimiter from the receiver copier.
   if (!pd->receiver) return (Eina_Slice){}; // Return empty slice if no receiver.
   return efl_io_copier_line_delimiter_get(pd->receiver);
}

EOLIAN static void
_efl_io_buffered_stream_timeout_inactivity_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, double seconds)
{
   // Set inactivity timeout on both receiver and sender copiers.
   if (pd->receiver)
     efl_io_copier_timeout_inactivity_set(pd->receiver, seconds);
   if (pd->sender)
     efl_io_copier_timeout_inactivity_set(pd->sender, seconds);
}

EOLIAN static double
_efl_io_buffered_stream_timeout_inactivity_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get inactivity timeout, preferring receiver's if available.
   if (pd->receiver)
     return efl_io_copier_timeout_inactivity_get(pd->receiver);
   if (pd->sender)
     return efl_io_copier_timeout_inactivity_get(pd->sender);
   return 0.0; // Default if no copiers.
}

EOLIAN static void
_efl_io_buffered_stream_read_chunk_size_set(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, size_t size)
{
   // This function seems to intend to set read chunk size for the receiver,
   // and buffer limit for the sender. The logic for sender might be a typo
   // or specific design choice.
   // For sender, it sets both buffer_limit and read_chunk_size.
   // For receiver, it also sets both.
   if (pd->sender)
     {
        efl_io_copier_buffer_limit_set(pd->sender, size);
        efl_io_copier_read_chunk_size_set(pd->sender, size);
     }

   // If there's no receiver, this part will operate on a NULL pd->receiver.
   // Assuming the check should be `if (pd->receiver)`
   if (pd->receiver) // Corrected: was `if (!pd->receiver)` which seems unintentional
     {
        efl_io_copier_buffer_limit_set(pd->receiver, size);
        efl_io_copier_read_chunk_size_set(pd->receiver, size);
     }
}

EOLIAN static size_t
_efl_io_buffered_stream_read_chunk_size_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the read chunk size from the receiver copier.
   if (!pd->receiver) return 0;
   return efl_io_copier_read_chunk_size_get(pd->receiver);
}

EOLIAN static size_t
_efl_io_buffered_stream_pending_write_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the amount of data pending in the outgoing (write) buffer queue.
   if (!pd->outgoing) return 0;
   return efl_io_queue_usage_get(pd->outgoing);
}

EOLIAN static size_t
_efl_io_buffered_stream_pending_read_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Get the amount of data available in the incoming (read) buffer queue.
   if (!pd->incoming) return 0;
   return efl_io_queue_usage_get(pd->incoming);
}

EOLIAN static void
_efl_io_buffered_stream_progress_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, size_t *pr, size_t *pw)
{
   uint64_t r = 0, w = 0;

   // Get total bytes read from receiver and written by sender.
   if (pd->sender) efl_io_copier_progress_get(pd->sender, NULL, &w, NULL);
   if (pd->receiver) efl_io_copier_progress_get(pd->receiver, &r, NULL, NULL);

   if (pr) *pr = r;
   if (pw) *pw = w;
}

EOLIAN static Eina_Slice
_efl_io_buffered_stream_slice_get(const Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   Eina_Slice slice = { };

   // Get a slice representing the currently available data in the incoming (read) buffer.
   if (pd->incoming)
     slice = efl_io_queue_slice_get(pd->incoming);

   return slice;
}

EOLIAN static void
_efl_io_buffered_stream_discard(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd, size_t amount)
{
   // Discard data from the incoming (read) buffer queue.
   if (!pd->incoming) return;
   efl_io_queue_discard(pd->incoming, amount);
}

EOLIAN static void
_efl_io_buffered_stream_clear(Eo *o EINA_UNUSED, Efl_Io_Buffered_Stream_Data *pd)
{
   // Clear all data from the incoming (read) buffer queue.
   if (!pd->incoming) return;
   efl_io_queue_clear(pd->incoming);
}

EOLIAN static void
_efl_io_buffered_stream_eos_mark(Eo *o, Efl_Io_Buffered_Stream_Data *pd)
{
   // Mark End-Of-Stream on the outgoing (write) queue.
   // This signals that no more data will be written to this stream.
   if (!pd->outgoing) return;
   DBG("%p mark eos", o);
   efl_io_queue_eos_mark(pd->outgoing);
}

EOLIAN static Eina_Bool
_efl_io_buffered_stream_flush(Eo *o, Efl_Io_Buffered_Stream_Data *pd, Eina_Bool may_block, Eina_Bool ignore_line_delimiter)
{
   size_t pending;
   Eina_Bool ret;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(efl_io_closer_closed_get(o), EINA_FALSE); // Cannot flush if closed.

   if (!pd->outgoing) return EINA_TRUE; // Nothing to flush if no outgoing queue.

   pending = efl_io_queue_usage_get(pd->outgoing);
   if (!pending) // Nothing to flush if queue is empty.
     return EINA_TRUE;

   // If the inner I/O is already closed, flushing might not be possible or meaningful.
   if (pd->is_closer && efl_io_closer_closed_get(pd->inner_io))
     {
        DBG("%p the inner I/O %p is already closed", o, pd->inner_io);
        return EINA_TRUE; // Or EINA_FALSE? Depends on desired behavior. Current code returns TRUE.
     }

   DBG("%p attempt to flush %zu bytes, may_block=%hhu, ignore_line_delimiter=%hhu...", o, pending, may_block, ignore_line_delimiter);
   ret = efl_io_copier_flush(pd->sender, may_block, ignore_line_delimiter);
   DBG("%p flushed, ret=%hhu, still pending=%zu", o, ret, efl_io_queue_usage_get(pd->outgoing));

   return ret;
}

#include "efl_io_buffered_stream.eo.c"
