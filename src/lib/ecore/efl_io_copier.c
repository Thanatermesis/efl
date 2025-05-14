/**
 * @file
 * @brief This file implements the Efl.Io.Copier class, which is responsible for
 * copying data from a source Efl.Io.Reader to a destination Efl.Io.Writer.
 * It handles buffering, progress reporting, and various events related to the
 * copy operation.
 */

#define EFL_IO_COPIER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#define MY_CLASS EFL_IO_COPIER_CLASS
#define DEF_READ_CHUNK_SIZE 4096

/**
 * @brief Private data structure for the Efl_Io_Copier class.
 */
typedef struct _Efl_Io_Copier_Data
{
   Efl_Io_Reader *source; /**< The source reader to copy data from. */
   Efl_Io_Writer *destination; /**< The destination writer to copy data to. */
   Eina_Future *inactivity_timer; /**< Future for handling inactivity timeout. */
   Eina_Future *job; /**< Future for the main copy job. */
   Eina_Binbuf *buf; /**< Internal buffer for storing data read from source before writing to destination. */
   Eina_Slice line_delimiter; /**< Optional delimiter for line-based events. If set, EFL_IO_COPIER_EVENT_LINE is emitted. */
   size_t buffer_limit; /**< Maximum size of the internal buffer. 0 means no limit. */
   size_t read_chunk_size; /**< Size of chunks to read from the source. */
   struct {
      uint64_t read; /**< Total bytes read from the source. */
      uint64_t written; /**< Total bytes written to the destination. */
      uint64_t total; /**< Total size of the source, if known (from Efl.Io.Sizer). */
   } progress; /**< Progress of the copy operation. */
   double timeout_inactivity; /**< Inactivity timeout in seconds. If no data is processed for this duration, an error event is emitted. */
   Eina_Bool closed; /**< Flag indicating if the copier has been closed. */
   Eina_Bool done; /**< Flag indicating if the copy operation is complete (EOS from source and buffer flushed). */
   Eina_Bool force_dispatch; /**< Flag to force dispatching data events even if line delimiter is not found. Used during flush or close. */
   Eina_Bool close_on_exec; /**< Flag to close the copier when exec() is called. */
   Eina_Bool close_on_invalidate; /**< Flag to close the copier when the object is invalidated. */
} Efl_Io_Copier_Data;

/**
 * @brief Writes data from the internal buffer to the destination.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
static void _efl_io_copier_write(Eo *o, Efl_Io_Copier_Data *pd);

/**
 * @brief Reads data from the source into the internal buffer.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
static void _efl_io_copier_read(Eo *o, Efl_Io_Copier_Data *pd);

/**
 * @brief Debug macro for logging copier state.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
#define _COPIER_DBG(o, pd) \
  do \
    { \
       if (eina_log_domain_level_check(_ecore_log_dom, EINA_LOG_LEVEL_DBG)) \
         { \
            DBG("copier={%p %s, refs=%d, closed=%d, done=%d, buf=%zd}", \
                o, \
                efl_class_name_get(efl_class_get(o)), \
                efl_ref_count(o), \
                efl_io_closer_closed_get(o), \
                pd->done, \
                pd->buf ? eina_binbuf_length_get(pd->buf): 0); \
            if (!pd->source) \
              DBG("source=NULL"); \
            else \
              DBG("source={%p %s, refs=%d, can_read=%d, eos=%d, closed=%d}", \
                  pd->source, \
                  efl_class_name_get(efl_class_get(pd->source)), \
                  efl_ref_count(pd->source), \
                  efl_io_reader_can_read_get(pd->source), \
                  efl_io_reader_eos_get(pd->source), \
                  efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE) ? \
                  efl_io_closer_closed_get(pd->source) : 0); \
            if (!pd->destination) \
              DBG("destination=NULL"); \
            else \
              DBG("destination={%p %s, refs=%d, can_write=%d, closed=%d}", \
                  pd->destination, \
                  efl_class_name_get(efl_class_get(pd->destination)), \
                  efl_ref_count(pd->destination), \
                  efl_io_writer_can_write_get(pd->destination), \
                  efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE) ? \
                  efl_io_closer_closed_get(pd->destination) : 0); \
         } \
    } \
  while (0)

/**
 * @brief Callback function triggered when the inactivity timer expires.
 *
 * Emits an EFL_IO_COPIER_EVENT_ERROR event with ETIMEDOUT.
 * @param o The Efl_Io_Copier object.
 * @param data User data (unused).
 * @param v The Eina_Value associated with the future (unused).
 * @return The input Eina_Value v.
 */
static Eina_Value
_efl_io_copier_timeout_inactivity_cb(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Eina_Error err = ETIMEDOUT;
   efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
   return v;
}

/**
 * @brief Reschedules the inactivity timer.
 *
 * If an existing timer is active, it's cancelled. A new timer is scheduled
 * if `pd->timeout_inactivity` is greater than 0.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
static void
_efl_io_copier_timeout_inactivity_reschedule(Eo *o, Efl_Io_Copier_Data *pd)
{
   if (pd->inactivity_timer) eina_future_cancel(pd->inactivity_timer);
   if (pd->timeout_inactivity <= 0.0) return;

   efl_future_then(o, efl_loop_timeout(efl_loop_get(o), pd->timeout_inactivity),
                   .success = _efl_io_copier_timeout_inactivity_cb,
                   .storage = &pd->inactivity_timer);
}

/**
 * @brief Main job function for the copier.
 *
 * This function attempts to read from the source and write to the destination.
 * It also handles progress updates and checks for completion (EOS).
 * It is scheduled as a future to run in the main loop.
 * @param o The Efl_Io_Copier object.
 * @param data User data (unused).
 * @param v The Eina_Value associated with the future (unused).
 * @return The input Eina_Value v.
 */
static Eina_Value
_efl_io_copier_job(Eo *o, void *data EINA_UNUSED, const Eina_Value v)
{
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   uint64_t old_read = pd->progress.read;
   uint64_t old_written = pd->progress.written;
   uint64_t old_total = pd->progress.total;

   _COPIER_DBG(o, pd);

   efl_ref(o);

   if (pd->source && efl_io_reader_can_read_get(pd->source))
     _efl_io_copier_read(o, pd);

   if (pd->destination && efl_io_writer_can_write_get(pd->destination))
     _efl_io_copier_write(o, pd);

   if ((old_read != pd->progress.read) ||
       (old_written != pd->progress.written) ||
       (old_total != pd->progress.total))
     {
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_PROGRESS, NULL);
        if (pd->closed) return v; /* cb may call close */
        _efl_io_copier_timeout_inactivity_reschedule(o, pd);
     }

   if (!pd->source || efl_io_reader_eos_get(pd->source))
     {
        if ((!pd->done) &&
            ((!pd->destination) || (eina_binbuf_length_get(pd->buf) == 0)))
          efl_io_copier_done_set(o, EINA_TRUE);
     }

   efl_unref(o);
   return v;
}

/**
 * @brief Schedules the main copier job if not already scheduled.
 *
 * If the object is invalidated, the job is run immediately. Otherwise,
 * it's scheduled as a future in the main loop.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
static void
_efl_io_copier_job_schedule(Eo *o, Efl_Io_Copier_Data *pd)
{
   if (pd->job) return;

   // When invalidated, no need to delay action
   if (efl_invalidated_get(o))
     {
        Eina_Value v = EINA_VALUE_EMPTY;

        v = _efl_io_copier_job(o, NULL, v);
        eina_value_flush(&v);
     }
   else
     {
        efl_future_then(o, efl_loop_job(efl_loop_get(o)),
                        .success = _efl_io_copier_job,
                        .storage = &pd->job);
     }
}

/* NOTE: the returned slice may be smaller than requested since the
 * internal binbuf may be modified from inside event calls.
 *
 * parameter slice_of_binbuf must have mem pointing to pd->binbuf
 *
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 * @param slice_of_binbuf A slice of the internal buffer that has been processed (read or written).
 *                        This slice is used to emit DATA and LINE events.
 * @return The potentially modified slice_of_binbuf. Callbacks might alter the buffer,
 *         so the returned slice reflects the valid portion after event processing.
 *         Returns a zero-length slice if the copier was closed during callbacks.
 */
static Eina_Slice
_efl_io_copier_dispatch_data_events(Eo *o, Efl_Io_Copier_Data *pd, Eina_Slice slice_of_binbuf)
{
   Eina_Slice tmp;
   size_t offset;

   tmp = eina_binbuf_slice_get(pd->buf);
   if ((slice_of_binbuf.bytes < tmp.bytes) ||
       (eina_slice_end_get(slice_of_binbuf) > eina_slice_end_get(tmp)))
     {
        CRI("slice_of_binbuf=" EINA_SLICE_FMT " must be inside binbuf=" EINA_SLICE_FMT,
            EINA_SLICE_PRINT(slice_of_binbuf), EINA_SLICE_PRINT(tmp));
        return (Eina_Slice){.mem = NULL, .len = 0};
     }

   offset = slice_of_binbuf.bytes - tmp.bytes;

   efl_event_callback_call(o, EFL_IO_COPIER_EVENT_DATA, &slice_of_binbuf);
   if (pd->closed) return (Eina_Slice){.mem = NULL, .len = 0}; /* cb may call close */

   /* user may have modified pd->buf, like calling
    * efl_io_copier_buffer_limit_set()
    */
   tmp = eina_binbuf_slice_get(pd->buf);
   if (offset <= tmp.len)
     {
        tmp.len -= offset;
        tmp.bytes += offset;
     }
   if (tmp.len > slice_of_binbuf.len)
     tmp.len = slice_of_binbuf.len;
   slice_of_binbuf = tmp;

   if (pd->line_delimiter.len > 0)
     {
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_LINE, &slice_of_binbuf);
        if (pd->closed) return (Eina_Slice){.mem = NULL, .len = 0}; /* cb may call close */

        /* user may have modified pd->buf, like calling
         * efl_io_copier_buffer_limit_set()
         */
        tmp = eina_binbuf_slice_get(pd->buf);
        if (offset <= tmp.len)
          {
             tmp.len -= offset;
             tmp.bytes += offset;
          }
        if (tmp.len > slice_of_binbuf.len)
          tmp.len = slice_of_binbuf.len;
        slice_of_binbuf = tmp;
     }

   return slice_of_binbuf;
}

static void
_efl_io_copier_read(Eo *o, Efl_Io_Copier_Data *pd)
{
   Eina_Rw_Slice rw_slice;
   Eina_Error err;
   size_t used, expand_size;

   EINA_SAFETY_ON_TRUE_RETURN(pd->closed);

   /* Try to read data from source into the buffer.
    * Respects buffer_limit and read_chunk_size.
    * Emits ERROR event on failure.
    * Emits DATA/LINE events if there's no destination.
    * Schedules the job for further processing.
    */

   expand_size = pd->read_chunk_size;
   used = eina_binbuf_length_get(pd->buf);
   if (pd->buffer_limit > 0)
     {
        if (pd->buffer_limit <= used)
          {
             return;
          }
        else if (pd->buffer_limit > used)
          {
             size_t available = pd->buffer_limit - used;
             if (expand_size > available)
               expand_size = available;
          }
     }

   rw_slice = eina_binbuf_expand(pd->buf, expand_size);
   if (rw_slice.len == 0)
     {
        err = ENOMEM;
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
        return;
     }

   err = efl_io_reader_read(pd->source, &rw_slice);
   if (err)
     {
        if (err != EAGAIN)
          efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
        return;
     }

   if (pd->closed) return; /* read(source) triggers cb, may call close */

   if (!eina_binbuf_use(pd->buf, rw_slice.len))
     {
        err = ENOMEM;
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
        return;
     }

   pd->progress.read += rw_slice.len;
   efl_io_copier_done_set(o, EINA_FALSE);

   if ((!pd->destination) && (eina_binbuf_length_get(pd->buf) > used))
     {
        /* Note: if there is a destination, dispatch data and line
         * from write since it will remove from binbuf and make it
         * simple to not repeat data that was already sent.
         *
         * however, if there is no destination, then emit the event
         * here.
         */
        _efl_io_copier_dispatch_data_events(o, pd, eina_rw_slice_slice_get(rw_slice));
     }

   _efl_io_copier_job_schedule(o, pd);
}

static void
_efl_io_copier_write(Eo *o, Efl_Io_Copier_Data *pd)
{
   Eina_Slice ro_slice;
   Eina_Error err;

   EINA_SAFETY_ON_TRUE_RETURN(pd->closed);
   EINA_SAFETY_ON_NULL_RETURN(pd->buf);

   /* Try to write data from the buffer to the destination.
    * If line_delimiter is set, it tries to write complete lines unless
    * force_dispatch is true or source is EOS and buffer is full.
    * Emits ERROR event on failure.
    * Dispatches DATA/LINE events for the written data.
    * Removes written data from the buffer.
    * Schedules the job for further processing.
    */

   ro_slice = eina_binbuf_slice_get(pd->buf);
   if (ro_slice.len == 0)
     {
        return;
     }

   if ((pd->line_delimiter.len > 0) && (!pd->force_dispatch) &&
       (pd->source && !efl_io_reader_eos_get(pd->source)))
     {
        const uint8_t *p = eina_slice_find(ro_slice, pd->line_delimiter);
        if (p)
          ro_slice.len = p - ro_slice.bytes + pd->line_delimiter.len;
        else if ((pd->buffer_limit == 0) || (ro_slice.len < pd->buffer_limit))
          {
             return;
          }
     }

   err = efl_io_writer_write(pd->destination, &ro_slice, NULL);
   if (err)
     {
        if (err != EAGAIN)
          efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
        return;
     }
   if (ro_slice.len == 0)
     return;

   pd->progress.written += ro_slice.len;

   if (pd->closed) return; /* write(destination) triggers cb, may call close */

   efl_io_copier_done_set(o, EINA_FALSE);

   /* Note: dispatch data and line from write since it will remove
    * from binbuf and make it simple to not repeat data that was
    * already sent.
    */
   ro_slice = _efl_io_copier_dispatch_data_events(o, pd, ro_slice);

   if (!eina_binbuf_remove(pd->buf, 0, ro_slice.len))
     {
        err = ENOMEM;
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
        return;
     }

   _efl_io_copier_job_schedule(o, pd);
}

/**
 * @brief Event callback for EFL_IO_READER_EVENT_CAN_READ_CHANGED on the source.
 *
 * Schedules the copier job if the source becomes readable.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_source_can_read_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (pd->closed) return;

   _COPIER_DBG(o, pd);

   if (efl_io_reader_can_read_get(pd->source))
     _efl_io_copier_job_schedule(o, pd);
}

/**
 * @brief Event callback for EFL_IO_READER_EVENT_EOS on the source.
 *
 * Schedules the copier job to process the EOS condition.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_source_eos(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (pd->closed) return;

   _COPIER_DBG(o, pd);

   _efl_io_copier_job_schedule(o, pd);
}

/**
 * @brief Applies the source size to the copier's progress and destination.
 *
 * Updates `pd->progress.total`. If the destination is an Efl_Io_Sizer,
 * it attempts to resize the destination. Emits EFL_IO_COPIER_EVENT_PROGRESS.
 * @param o The Efl_Io_Copier object.
 * @param pd The private data of the Efl_Io_Copier object.
 */
static void
_efl_io_copier_source_size_apply(Eo *o, Efl_Io_Copier_Data *pd)
{
   if (pd->closed) return;
   pd->progress.total = efl_io_sizer_size_get(pd->source);

   _COPIER_DBG(o, pd);

   if (pd->destination && efl_isa(pd->destination, EFL_IO_SIZER_MIXIN))
     efl_io_sizer_resize(pd->destination, pd->progress.total);

   efl_event_callback_call(o, EFL_IO_COPIER_EVENT_PROGRESS, NULL);
}

/**
 * @brief Event callback for EFL_IO_SIZER_EVENT_SIZE_CHANGED on the source.
 *
 * Calls _efl_io_copier_source_size_apply to update progress and destination size.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_source_resized(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   _efl_io_copier_source_size_apply(o, pd);
}

/**
 * @brief Event callback for EFL_IO_CLOSER_EVENT_CLOSED on the source.
 *
 * Schedules the copier job to handle the source closure.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_source_closed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (pd->closed) return;

   _COPIER_DBG(o, pd);

   _efl_io_copier_job_schedule(o, pd);
}

/**
 * @brief Array of callbacks for source events.
 */
EFL_CALLBACKS_ARRAY_DEFINE(source_cbs,
                          { EFL_IO_READER_EVENT_CAN_READ_CHANGED, _efl_io_copier_source_can_read_changed },
                          { EFL_IO_READER_EVENT_EOS, _efl_io_copier_source_eos });

EOLIAN static Efl_Io_Reader *
_efl_io_copier_source_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->source;
}

EOLIAN static void
_efl_io_copier_source_set(Eo *o, Efl_Io_Copier_Data *pd, Efl_Io_Reader *source)
{
   if (pd->source == source) return;

   if (pd->source)
     {
        if (efl_isa(pd->source, EFL_IO_SIZER_MIXIN))
          {
             efl_event_callback_del(pd->source, EFL_IO_SIZER_EVENT_SIZE_CHANGED,
                                    _efl_io_copier_source_resized, o);
             pd->progress.total = 0;
          }
        if (efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE))
          {
             efl_event_callback_del(pd->source, EFL_IO_CLOSER_EVENT_CLOSED,
                                    _efl_io_copier_source_closed, o);
          }
        efl_event_callback_array_del(pd->source, source_cbs(), o);
        efl_unref(pd->source);
        pd->source = NULL;
     }

   if (source)
     {
        EINA_SAFETY_ON_TRUE_RETURN(pd->closed);
        pd->source = efl_ref(source);
        efl_event_callback_array_add(pd->source, source_cbs(), o);

        if (efl_isa(pd->source, EFL_IO_SIZER_MIXIN))
          {
             efl_event_callback_add(pd->source, EFL_IO_SIZER_EVENT_SIZE_CHANGED,
                                    _efl_io_copier_source_resized, o);
             _efl_io_copier_source_size_apply(o, pd);
          }

        if (efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE))
          {
             efl_io_closer_close_on_exec_set(pd->source, efl_io_closer_close_on_exec_get(o));
             efl_io_closer_close_on_invalidate_set(pd->source, efl_io_closer_close_on_invalidate_get(o));
             efl_event_callback_add(pd->source, EFL_IO_CLOSER_EVENT_CLOSED,
                                     _efl_io_copier_source_closed, o);
          }
     }
}

/**
 * @brief Event callback for EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED on the destination.
 *
 * Schedules the copier job if the destination becomes writable.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_destination_can_write_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (pd->closed) return;

   _COPIER_DBG(o, pd);

   if (efl_io_writer_can_write_get(pd->destination))
     _efl_io_copier_job_schedule(o, pd);
}

/**
 * @brief Event callback for EFL_IO_CLOSER_EVENT_CLOSED on the destination.
 *
 * If the internal buffer is empty, sets the copier to done.
 * Otherwise, if there's pending data, emits an EFL_IO_COPIER_EVENT_ERROR
 * with EBADF, as data cannot be written to a closed destination.
 * @param data The Efl_Io_Copier object.
 * @param event The event information (unused).
 */
static void
_efl_io_copier_destination_closed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Eo *o = data;
   Efl_Io_Copier_Data *pd = efl_data_scope_get(o, MY_CLASS);
   if (pd->closed) return;

   _COPIER_DBG(o, pd);

   if (eina_binbuf_length_get(pd->buf) == 0)
     {
        if (!pd->done)
          efl_io_copier_done_set(o, EINA_TRUE);
     }
   else
     {
        Eina_Error err = EBADF;
        if (pd->inactivity_timer) eina_future_cancel(pd->inactivity_timer);
        WRN("copier %p destination %p closed with %zd bytes pending...",
            o, pd->destination, eina_binbuf_length_get(pd->buf));
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_ERROR, &err);
     }
}

/**
 * @brief Array of callbacks for destination events.
 */
EFL_CALLBACKS_ARRAY_DEFINE(destination_cbs,
                          { EFL_IO_WRITER_EVENT_CAN_WRITE_CHANGED, _efl_io_copier_destination_can_write_changed });

EOLIAN static Efl_Io_Writer *
_efl_io_copier_destination_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->destination;
}

EOLIAN static void
_efl_io_copier_destination_set(Eo *o, Efl_Io_Copier_Data *pd, Efl_Io_Writer *destination)
{
   if (pd->destination == destination) return;

   if (pd->destination)
     {
        efl_event_callback_array_del(pd->destination, destination_cbs(), o);
        if (efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE))
          {
             efl_event_callback_del(pd->destination, EFL_IO_CLOSER_EVENT_CLOSED,
                                    _efl_io_copier_destination_closed, o);
          }
        efl_unref(pd->destination);
        pd->destination = NULL;
     }

   if (destination)
     {
        EINA_SAFETY_ON_TRUE_RETURN(pd->closed);
        pd->destination = efl_ref(destination);
        efl_event_callback_array_add(pd->destination, destination_cbs(), o);

        if (efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE))
          {
             efl_io_closer_close_on_exec_set(pd->destination, efl_io_closer_close_on_exec_get(o));
             efl_io_closer_close_on_invalidate_set(pd->destination, efl_io_closer_close_on_invalidate_get(o));
             efl_event_callback_add(pd->destination, EFL_IO_CLOSER_EVENT_CLOSED,
                                     _efl_io_copier_destination_closed, o);
          }
        if (efl_isa(pd->destination, EFL_IO_SIZER_MIXIN) &&
            pd->source && efl_isa(pd->source, EFL_IO_SIZER_MIXIN))
          {
             efl_io_sizer_resize(pd->destination, pd->progress.total);
          }
     }
}

EOLIAN static void
_efl_io_copier_buffer_limit_set(Eo *o, Efl_Io_Copier_Data *pd, size_t size)
{
   size_t used;

   EINA_SAFETY_ON_TRUE_RETURN(pd->closed);

   if (pd->buffer_limit == size) return;
   pd->buffer_limit = size;
   if (size == 0) return;

   used = eina_binbuf_length_get(pd->buf);
   if (used > size) eina_binbuf_remove(pd->buf, size, used);
   if (pd->read_chunk_size > size) efl_io_copier_read_chunk_size_set(o, size);
}

EOLIAN static size_t
_efl_io_copier_buffer_limit_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->buffer_limit;
}

EOLIAN static void
_efl_io_copier_line_delimiter_set(Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd, Eina_Slice slice)
{
   if (pd->line_delimiter.mem == slice.mem)
     {
        pd->line_delimiter.len = slice.len;
        return;
     }

   free((void *)pd->line_delimiter.mem);
   if (slice.len == 0)
     {
        pd->line_delimiter.mem = NULL;
        pd->line_delimiter.len = 0;
     }
   else
     {
        Eina_Rw_Slice rw_slice = eina_slice_dup(slice);
        pd->line_delimiter = eina_rw_slice_slice_get(rw_slice);
     }
}

EOLIAN static Eina_Slice
_efl_io_copier_line_delimiter_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->line_delimiter;
}


EOLIAN static void
_efl_io_copier_read_chunk_size_set(Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd, size_t size)
{
   EINA_SAFETY_ON_TRUE_RETURN(pd->closed);

   if (size == 0) size = DEF_READ_CHUNK_SIZE;
   pd->read_chunk_size = size;
}

EOLIAN static size_t
_efl_io_copier_read_chunk_size_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->read_chunk_size > 0 ? pd->read_chunk_size : DEF_READ_CHUNK_SIZE;
}

EOLIAN static Eina_Error
_efl_io_copier_efl_io_closer_close(Eo *o, Efl_Io_Copier_Data *pd)
{
   Eina_Error err = 0, r;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(pd->closed, EINVAL);

   _COPIER_DBG(o, pd);

   while (pd->buf)
     {
        size_t pending = eina_binbuf_length_get(pd->buf);
        if (pending == 0) break;
        else if (pd->destination && efl_io_writer_can_write_get(pd->destination))
          {
             DBG("copier %p destination %p closed with %zd bytes pending, do final write...",
                 o, pd->destination, pending);
             pd->force_dispatch = EINA_TRUE;
             _efl_io_copier_write(o, pd);
             pd->force_dispatch = EINA_FALSE;
          }
        else if (!pd->destination)
          {
             Eina_Slice binbuf_slice = eina_binbuf_slice_get(pd->buf);
             DBG("copier %p destination %p closed with %zd bytes pending, dispatch events...",
                 o, pd->destination, pending);
             _efl_io_copier_dispatch_data_events(o, pd, binbuf_slice);
             break;
          }
        else
          {
             DBG("copier %p destination %p closed with %zd bytes pending...",
                 o, pd->destination, pending);
             break;
          }
     }

   if (pd->job)
     eina_future_cancel(pd->job);

   if (pd->inactivity_timer)
     eina_future_cancel(pd->inactivity_timer);

   if (!pd->done)
     efl_io_copier_done_set(o, EINA_TRUE);

   if (pd->source)
     {
        if (efl_isa(pd->source, EFL_IO_SIZER_MIXIN))
          {
             efl_event_callback_del(pd->source, EFL_IO_SIZER_EVENT_SIZE_CHANGED,
                                    _efl_io_copier_source_resized, o);
             pd->progress.total = 0;
          }
        efl_event_callback_array_del(pd->source, source_cbs(), o);
        if (efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE) &&
            !efl_io_closer_closed_get(pd->source))
          {
             efl_event_callback_del(pd->source, EFL_IO_CLOSER_EVENT_CLOSED,
                                    _efl_io_copier_source_closed, o);
             err = efl_io_closer_close(pd->source);
          }
     }

   if (pd->destination)
     {
        efl_event_callback_array_del(pd->destination, destination_cbs(), o);
        if (efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE) &&
            !efl_io_closer_closed_get(pd->destination))
          {
             efl_event_callback_del(pd->destination, EFL_IO_CLOSER_EVENT_CLOSED,
                                    _efl_io_copier_destination_closed, o);
             r = efl_io_closer_close(pd->destination);
             if (!err) err = r;
          }
     }

   pd->closed = EINA_TRUE;
   efl_event_callback_call(o, EFL_IO_CLOSER_EVENT_CLOSED, NULL);

   if (pd->buf)
     {
        eina_binbuf_free(pd->buf);
        pd->buf = NULL;
     }

   return err;
}

EOLIAN static Eina_Bool
_efl_io_copier_efl_io_closer_closed_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->closed;
}

EOLIAN static void
_efl_io_copier_progress_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd, uint64_t *read, uint64_t *written, uint64_t *total)
{
   if (read) *read = pd->progress.read;
   if (written) *written = pd->progress.written;
   if (total) *total = pd->progress.total;
}

EOLIAN static Eina_Binbuf *
_efl_io_copier_binbuf_steal(Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   Eina_Binbuf *ret = pd->buf;
   pd->buf = eina_binbuf_new();
   return ret;
}

EOLIAN static void
_efl_io_copier_timeout_inactivity_set(Eo *o, Efl_Io_Copier_Data *pd, double seconds)
{
   pd->timeout_inactivity = seconds;
   _efl_io_copier_timeout_inactivity_reschedule(o, pd);
}

EOLIAN static double
_efl_io_copier_timeout_inactivity_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->timeout_inactivity;
}

EOLIAN static Eina_Bool
_efl_io_copier_flush(Eo *o, Efl_Io_Copier_Data *pd, Eina_Bool may_block, Eina_Bool ignore_line_delimiter)
{
   uint64_t old_read = pd->progress.read;
   uint64_t old_written = pd->progress.written;
   uint64_t old_total = pd->progress.total;

   _COPIER_DBG(o, pd);

   if (pd->source && !efl_io_reader_eos_get(pd->source))
     {
        if (may_block || efl_io_reader_can_read_get(pd->source))
          _efl_io_copier_read(o, pd);
     }

   if (pd->destination)
     {
        if (may_block || efl_io_writer_can_write_get(pd->destination))
          {
             pd->force_dispatch = ignore_line_delimiter;
             _efl_io_copier_write(o, pd);
             pd->force_dispatch = EINA_FALSE;
          }
     }
   else if (ignore_line_delimiter && pd->buf)
     {
        size_t pending = eina_binbuf_length_get(pd->buf);
        if (pending)
          {
             Eina_Slice binbuf_slice = eina_binbuf_slice_get(pd->buf);
             _efl_io_copier_dispatch_data_events(o, pd, binbuf_slice);
          }
     }

   if ((old_read != pd->progress.read) ||
       (old_written != pd->progress.written) ||
       (old_total != pd->progress.total))
     {
        efl_event_callback_call(o, EFL_IO_COPIER_EVENT_PROGRESS, NULL);
        if (pd->closed) return EINA_TRUE; /* cb may call close */
        _efl_io_copier_timeout_inactivity_reschedule(o, pd);
     }

   if (!pd->source || efl_io_reader_eos_get(pd->source))
     {
        if ((!pd->done) &&
            ((!pd->destination) || (eina_binbuf_length_get(pd->buf) == 0)))
          efl_io_copier_done_set(o, EINA_TRUE);
     }

   return pd->done;
}

EOLIAN static size_t
_efl_io_copier_pending_size_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->buf ? eina_binbuf_length_get(pd->buf) : 0;
}

EOLIAN static Eina_Bool
_efl_io_copier_done_get(const Eo *o, Efl_Io_Copier_Data *pd)
{
   DBG("%p done=%d pending=%zd source={%p %s, eos=%d, closed=%d}, destination={%p %s, closed=%d}",
       o, pd->done,
       pd->buf ? eina_binbuf_length_get(pd->buf) : 0,
       pd->source,
       pd->source ? efl_class_name_get(pd->source) : "",
       pd->source ? efl_io_reader_eos_get(pd->source) : 1,
       pd->source ? (efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE) ? efl_io_closer_closed_get(pd->source) : 0) : 1,
       pd->destination,
       pd->destination ? efl_class_name_get(pd->destination) : "",
       pd->destination ? (efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE) ? efl_io_closer_closed_get(pd->destination) : 0) : 1);

   return pd->done;
}

EOLIAN static void
_efl_io_copier_done_set(Eo *o, Efl_Io_Copier_Data *pd, Eina_Bool value)
{
   if (pd->done == value) return;
   pd->done = value;
   if (!value) return;
   if (pd->inactivity_timer) eina_future_cancel(pd->inactivity_timer);
   efl_event_callback_call(o, EFL_IO_COPIER_EVENT_DONE, NULL);
}


EOLIAN static Eo *
_efl_io_copier_efl_object_constructor(Eo *o, Efl_Io_Copier_Data *pd)
{
   pd->buf = eina_binbuf_new();
   pd->close_on_exec = EINA_TRUE;
   pd->close_on_invalidate = EINA_TRUE;
   pd->timeout_inactivity = 0.0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->buf, NULL);

   return efl_constructor(efl_super(o, MY_CLASS));
}

EOLIAN static Eo *
_efl_io_copier_efl_object_finalize(Eo *o, Efl_Io_Copier_Data *pd)
{
   if (pd->read_chunk_size == 0)
     efl_io_copier_read_chunk_size_set(o, DEF_READ_CHUNK_SIZE);

   if (!efl_loop_get(o))
     {
        ERR("Set a loop provider as parent of this copier!");
        return NULL;
     }

   if ((pd->source && efl_io_reader_can_read_get(pd->source)) ||
       (pd->destination && efl_io_writer_can_write_get(pd->destination)))
     _efl_io_copier_job_schedule(o, pd);

   _COPIER_DBG(o, pd);

   return efl_finalize(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_io_copier_efl_object_invalidate(Eo *o, Efl_Io_Copier_Data *pd EINA_UNUSED)
{
   if (efl_io_closer_close_on_invalidate_get(o) &&
       (!efl_io_closer_closed_get(o)))
     {
        efl_event_freeze(o);
        efl_io_closer_close(o);
        efl_event_thaw(o);
     }

   efl_io_copier_source_set(o, NULL);
   efl_io_copier_destination_set(o, NULL);

   efl_invalidate(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_io_copier_efl_object_destructor(Eo *o, Efl_Io_Copier_Data *pd)
{
   _COPIER_DBG(o, pd);

   if (pd->job)
     eina_future_cancel(pd->job);

   if (pd->inactivity_timer)
     eina_future_cancel(pd->inactivity_timer);

   efl_destructor(efl_super(o, MY_CLASS));

   if (pd->buf)
     {
        eina_binbuf_free(pd->buf);
        pd->buf = NULL;
     }

   if (pd->line_delimiter.mem)
     {
        free((void *)pd->line_delimiter.mem);
        pd->line_delimiter.mem = NULL;
        pd->line_delimiter.len = 0;
     }
}

EOLIAN static Eina_Bool
_efl_io_copier_efl_io_closer_close_on_exec_set(Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd, Eina_Bool close_on_exec)
{
   if (pd->close_on_exec == close_on_exec) return EINA_TRUE;
   pd->close_on_exec = close_on_exec;

   if (pd->source && efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE))
     efl_io_closer_close_on_exec_set(pd->source, close_on_exec);

   if (pd->destination && efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE))
     efl_io_closer_close_on_exec_set(pd->destination, close_on_exec);

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl_io_copier_efl_io_closer_close_on_exec_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->close_on_exec;
}

EOLIAN static void
_efl_io_copier_efl_io_closer_close_on_invalidate_set(Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd, Eina_Bool close_on_invalidate)
{
   if (pd->close_on_invalidate == close_on_invalidate) return;
   pd->close_on_invalidate = close_on_invalidate;

   if (pd->source && efl_isa(pd->source, EFL_IO_CLOSER_INTERFACE))
     efl_io_closer_close_on_invalidate_set(pd->source, close_on_invalidate);

   if (pd->destination && efl_isa(pd->destination, EFL_IO_CLOSER_INTERFACE))
     efl_io_closer_close_on_invalidate_set(pd->destination, close_on_invalidate);
}

EOLIAN static Eina_Bool
_efl_io_copier_efl_io_closer_close_on_invalidate_get(const Eo *o EINA_UNUSED, Efl_Io_Copier_Data *pd)
{
   return pd->close_on_invalidate;
}

#include "efl_io_copier.eo.c"
