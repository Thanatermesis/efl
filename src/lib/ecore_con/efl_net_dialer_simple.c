#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Private data structure for Efl_Net_Dialer_Simple.
 */
typedef struct
{
   const Efl_Class *inner_class; /**< The class to be used for the inner dialer object. Set via efl_net_dialer_simple_inner_class_set(). */
   Eina_Stringshare *proxy_url; /**< Stores the proxy URL if set before the inner dialer is created. */
   double dial_timeout; /**< Stores the dial timeout if set before the inner dialer is created. */
   double timeout_inactivity; /**< Stores the inactivity timeout if set before the inner dialer is created. */
   size_t max_queue_size_input; /**< Stores the max input queue size if set before the inner dialer is created. */
   size_t max_queue_size_output; /**< Stores the max output queue size if set before the inner dialer is created. */
   size_t read_chunk_size; /**< Stores the read chunk size if set before the inner dialer is created. */
   Eina_Slice line_delimiter; /**< Stores the line delimiter if set before the inner dialer is created. The memory for this slice is managed internally. */
   /**
    * @brief Flags to indicate which properties have been set before the inner dialer
    * was created and thus need to be applied once it's available.
    */
   struct {
      Eina_Bool proxy_url; /**< True if proxy_url is pending. */
      Eina_Bool dial_timeout; /**< True if dial_timeout is pending. */
      Eina_Bool timeout_inactivity; /**< True if timeout_inactivity is pending. */
      Eina_Bool max_queue_size_input; /**< True if max_queue_size_input is pending. */
      Eina_Bool max_queue_size_output; /**< True if max_queue_size_output is pending. */
      Eina_Bool read_chunk_size; /**< True if read_chunk_size is pending. */
      Eina_Bool line_delimiter; /**< True if line_delimiter is pending. */
   } pending;
} Efl_Net_Dialer_Simple_Data;

#define MY_CLASS EFL_NET_DIALER_SIMPLE_CLASS

/**
 * @brief Finalizes the Efl_Net_Dialer_Simple object.
 *
 * This function is called when the object is being finalized. If an inner I/O
 * object (dialer) has not been set yet, it attempts to create one using the
 * class specified by efl_net_dialer_simple_inner_class_set().
 * It ensures that the created dialer implements the Efl.Net.Dialer interface.
 *
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The finalized Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_net_dialer_simple_efl_object_finalize(Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   if (efl_io_buffered_stream_inner_io_get(o)) goto end;

   if (!pd->inner_class)
     {
        ERR("no valid dialer was set with efl_io_buffered_stream_inner_io_set() and no class set with efl_net_dialer_simple_inner_class_set()!");
        return NULL;
     }
   else
     {
        Eo *dialer = efl_add(pd->inner_class, o);
        EINA_SAFETY_ON_NULL_RETURN_VAL(dialer, NULL);

        if (!efl_isa(dialer, EFL_NET_DIALER_INTERFACE))
          {
             ERR("class %s=%p doesn't implement Efl.Net.Dialer interface!", efl_class_name_get(pd->inner_class), pd->inner_class);
             efl_del(dialer);
             return NULL;
          }
        DBG("created new inner dialer %p (%s)", dialer, efl_class_name_get(efl_class_get(dialer)));

        efl_io_buffered_stream_inner_io_set(o, dialer);
     }

 end:
   return efl_finalize(efl_super(o, MY_CLASS));
}

/**
 * @brief Invalidates the Efl_Net_Dialer_Simple object.
 *
 * This function is called when the object is being invalidated. It performs
 * cleanup related to the inner I/O object, such as removing event forwarders
 * and clearing its parent if this object was its parent.
 *
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object (unused).
 */
EOLIAN static void
_efl_net_dialer_simple_efl_object_invalidate(Eo *o, Efl_Net_Dialer_Simple_Data *pd EINA_UNUSED)
{
   Eo *inner_io;

   inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (inner_io)
     {
        efl_event_callback_forwarder_del(inner_io, EFL_NET_DIALER_EVENT_DIALER_ERROR, o);
        efl_event_callback_forwarder_del(inner_io, EFL_NET_DIALER_EVENT_DIALER_RESOLVED, o);
        efl_event_callback_forwarder_del(inner_io, EFL_NET_DIALER_EVENT_DIALER_CONNECTED, o);
        if (efl_parent_get(inner_io) == o)
          efl_parent_set(inner_io, NULL);
     }

   efl_invalidate(efl_super(o, EFL_NET_DIALER_SIMPLE_CLASS));
}

/**
 * @brief Destroys the Efl_Net_Dialer_Simple object.
 *
 * This function is called when the object is being destroyed. It releases
 * resources held by the private data, such as the stringshared proxy URL
 * and the memory allocated for the line delimiter.
 *
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_object_destructor(Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   if (pd->inner_class) pd->inner_class = NULL;

   eina_stringshare_replace(&pd->proxy_url, NULL);
   if (pd->line_delimiter.mem)
     {
        free((void *)pd->line_delimiter.mem);
        pd->line_delimiter.mem = NULL;
     }

   efl_destructor(efl_super(o, EFL_NET_DIALER_SIMPLE_CLASS));
}

/**
 * @brief Sets the inner I/O object for the Efl_Net_Dialer_Simple.
 *
 * This function sets the underlying I/O object that will handle the actual
 * dialing operations. It ensures the provided object implements the
 * Efl.Net.Dialer interface. It also sets up event forwarders for dialer-specific
 * events and applies any pending properties that were set on this simple dialer
 * before the inner I/O object was available.
 *
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param io The inner I/O object to set. Must implement Efl.Net.Dialer.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_inner_io_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, Efl_Object *io)
{
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(io, EFL_NET_DIALER_INTERFACE));
   efl_io_buffered_stream_inner_io_set(efl_super(o, MY_CLASS), io);
   efl_event_callback_forwarder_add(io, EFL_NET_DIALER_EVENT_DIALER_ERROR, o);
   efl_event_callback_forwarder_add(io, EFL_NET_DIALER_EVENT_DIALER_RESOLVED, o);
   efl_event_callback_forwarder_add(io, EFL_NET_DIALER_EVENT_DIALER_CONNECTED, o);
   /* apply pending dialer values */
   if (pd->pending.proxy_url)
     {
        pd->pending.proxy_url = EINA_FALSE;
        efl_net_dialer_proxy_set(io, pd->proxy_url);
        eina_stringshare_replace(&pd->proxy_url, NULL);
     }
   if (pd->pending.dial_timeout)
     {
        pd->pending.dial_timeout = EINA_FALSE;
        efl_net_dialer_timeout_dial_set(io, pd->dial_timeout);
     }

   /* apply pending io buffered stream (own) values */
   if (pd->pending.timeout_inactivity)
     {
        pd->pending.timeout_inactivity = EINA_FALSE;
        efl_io_buffered_stream_timeout_inactivity_set(o, pd->timeout_inactivity);
     }
   if (pd->pending.max_queue_size_input)
     {
        pd->pending.max_queue_size_input = EINA_FALSE;
        efl_io_buffered_stream_max_queue_size_input_set(o, pd->max_queue_size_input);
     }
   if (pd->pending.max_queue_size_output)
     {
        pd->pending.max_queue_size_output = EINA_FALSE;
        efl_io_buffered_stream_max_queue_size_output_set(o, pd->max_queue_size_output);
     }
   if (pd->pending.read_chunk_size)
     {
        pd->pending.read_chunk_size = EINA_FALSE;
        efl_io_buffered_stream_read_chunk_size_set(o, pd->read_chunk_size);
     }
   if (pd->pending.line_delimiter)
     {
        pd->pending.line_delimiter = EINA_FALSE;
        efl_io_buffered_stream_line_delimiter_set(o, pd->line_delimiter);
        free((void *)pd->line_delimiter.mem);
        pd->line_delimiter.mem = NULL;
     }
}

/**
 * @brief Dials the given address using the inner dialer.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd Private data (unused).
 * @param address The address to dial (e.g., "hostname:port").
 * @return 0 on success, a system error code otherwise.
 */
EOLIAN static Eina_Error
_efl_net_dialer_simple_efl_net_dialer_dial(Eo *o, Efl_Net_Dialer_Simple_Data *pd EINA_UNUSED, const char *address)
{
   return efl_net_dialer_dial(efl_io_buffered_stream_inner_io_get(o), address);
}

/**
 * @brief Gets the address being dialed by the inner dialer.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd Private data (unused).
 * @return The address string, or NULL if not dialing.
 */
EOLIAN static const char *
_efl_net_dialer_simple_efl_net_dialer_address_dial_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd EINA_UNUSED)
{
   return efl_net_dialer_address_dial_get(efl_io_buffered_stream_inner_io_get(o));
}

/**
 * @brief Checks if the inner dialer is connected.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd Private data (unused).
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_net_dialer_simple_efl_net_dialer_connected_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd EINA_UNUSED)
{
   return efl_net_dialer_connected_get(efl_io_buffered_stream_inner_io_get(o));
}

/**
 * @brief Sets the proxy URL for the dialer.
 * If the inner dialer is not yet created, the URL is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param proxy_url The proxy URL string (e.g., "socks5://user:pass@host:port").
 */
EOLIAN static void
_efl_net_dialer_simple_efl_net_dialer_proxy_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, const char *proxy_url)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        eina_stringshare_replace(&pd->proxy_url, proxy_url);
        pd->pending.proxy_url = EINA_TRUE;
        return;
     }
   efl_net_dialer_proxy_set(inner_io, proxy_url);
}

/**
 * @brief Gets the proxy URL for the dialer.
 * If the inner dialer is not yet created, returns the stored URL.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The proxy URL string.
 */
EOLIAN static const char *
_efl_net_dialer_simple_efl_net_dialer_proxy_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->proxy_url;
   return efl_net_dialer_proxy_get(inner_io);
}

/**
 * @brief Sets the dial timeout for the dialer.
 * If the inner dialer is not yet created, the timeout is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param seconds The timeout in seconds.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_net_dialer_timeout_dial_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, double seconds)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        pd->dial_timeout = seconds;
        pd->pending.dial_timeout = EINA_TRUE;
        return;
     }
   efl_net_dialer_timeout_dial_set(inner_io, seconds);
}

/**
 * @brief Gets the dial timeout for the dialer.
 * If the inner dialer is not yet created, returns the stored timeout.
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The timeout in seconds.
 */
EOLIAN static double
_efl_net_dialer_simple_efl_net_dialer_timeout_dial_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->dial_timeout;
   return efl_net_dialer_timeout_dial_get(inner_io);
}

/**
 * @brief Sets the inactivity timeout for the buffered stream.
 * If the inner dialer (which is also the inner I/O for the buffered stream)
 * is not yet created, the timeout is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param seconds The timeout in seconds.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_timeout_inactivity_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, double seconds)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        pd->timeout_inactivity = seconds;
        pd->pending.timeout_inactivity = EINA_TRUE;
        return;
     }
   efl_io_buffered_stream_timeout_inactivity_set(efl_super(o, MY_CLASS), seconds);
}

/**
 * @brief Gets the inactivity timeout for the buffered stream.
 * If the inner dialer is not yet created, returns the stored timeout.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The timeout in seconds.
 */
EOLIAN static double
_efl_net_dialer_simple_efl_io_buffered_stream_timeout_inactivity_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->timeout_inactivity;
   return efl_io_buffered_stream_timeout_inactivity_get(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the maximum input queue size for the buffered stream.
 * If the inner dialer is not yet created, the size is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param size The maximum queue size in bytes.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_max_queue_size_input_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, size_t size)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        pd->max_queue_size_input = size;
        pd->pending.max_queue_size_input = EINA_TRUE;
        return;
     }
   efl_io_buffered_stream_max_queue_size_input_set(efl_super(o, MY_CLASS), size);
}

/**
 * @brief Gets the maximum input queue size for the buffered stream.
 * If the inner dialer is not yet created, returns the stored size.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The maximum queue size in bytes.
 */
EOLIAN static size_t
_efl_net_dialer_simple_efl_io_buffered_stream_max_queue_size_input_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->max_queue_size_input;
   return efl_io_buffered_stream_max_queue_size_input_get(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the maximum output queue size for the buffered stream.
 * If the inner dialer is not yet created, the size is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param size The maximum queue size in bytes.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_max_queue_size_output_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, size_t size)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        pd->max_queue_size_output = size;
        pd->pending.max_queue_size_output = EINA_TRUE;
        return;
     }
   efl_io_buffered_stream_max_queue_size_output_set(efl_super(o, MY_CLASS), size);
}

/**
 * @brief Gets the maximum output queue size for the buffered stream.
 * If the inner dialer is not yet created, returns the stored size.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The maximum queue size in bytes.
 */
EOLIAN static size_t
_efl_net_dialer_simple_efl_io_buffered_stream_max_queue_size_output_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->max_queue_size_output;
   return efl_io_buffered_stream_max_queue_size_output_get(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the read chunk size for the buffered stream.
 * If the inner dialer is not yet created, the size is stored and applied later.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param size The read chunk size in bytes.
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_read_chunk_size_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, size_t size)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        pd->read_chunk_size = size;
        pd->pending.read_chunk_size = EINA_TRUE;
        return;
     }
   efl_io_buffered_stream_read_chunk_size_set(efl_super(o, MY_CLASS), size);
}

/**
 * @brief Gets the read chunk size for the buffered stream.
 * If the inner dialer is not yet created, returns the stored size.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The read chunk size in bytes.
 */
EOLIAN static size_t
_efl_net_dialer_simple_efl_io_buffered_stream_read_chunk_size_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->read_chunk_size;
   return efl_io_buffered_stream_read_chunk_size_get(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the line delimiter for the buffered stream.
 * If the inner dialer is not yet created, the delimiter is copied and stored,
 * then applied later. The provided slice's memory is not used directly after this call
 * if the inner_io is not yet set.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param slice The slice representing the line delimiter. e.g. EINA_SLICE_STR("\r\n").
 */
EOLIAN static void
_efl_net_dialer_simple_efl_io_buffered_stream_line_delimiter_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, Eina_Slice slice)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);

   if (!inner_io)
     {
        free((void *)pd->line_delimiter.mem);
        if (!slice.len)
          {
             pd->line_delimiter.mem = NULL;
             pd->line_delimiter.len = 0;
          }
        else
          {
             char *mem;
             pd->line_delimiter.mem = mem = malloc(slice.len + 1);
             EINA_SAFETY_ON_NULL_RETURN(pd->line_delimiter.mem);
             memcpy(mem, slice.mem, slice.len);
             mem[slice.len] = '\0';
             pd->line_delimiter.len = slice.len;
          }

        pd->pending.line_delimiter = EINA_TRUE;
        return;
     }
   efl_io_buffered_stream_line_delimiter_set(efl_super(o, MY_CLASS), slice);
}

/**
 * @brief Gets the line delimiter for the buffered stream.
 * If the inner dialer is not yet created, returns the stored delimiter.
 * @param o The Efl_Net_Dialer_Simple object (acting as Efl_Io_Buffered_Stream).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The slice representing the line delimiter.
 */
EOLIAN static Eina_Slice
_efl_net_dialer_simple_efl_io_buffered_stream_line_delimiter_get(const Eo *o, Efl_Net_Dialer_Simple_Data *pd)
{
   Eo *inner_io = efl_io_buffered_stream_inner_io_get(o);
   if (!inner_io) return pd->line_delimiter;
   return efl_io_buffered_stream_line_delimiter_get(efl_super(o, MY_CLASS));
}

/**
 * @brief Sets the Efl_Class to be used for creating the inner dialer object.
 * This must be called before the Efl_Net_Dialer_Simple object is finalized
 * if an inner I/O object is not set explicitly via
 * efl_io_buffered_stream_inner_io_set(). The provided class must implement
 * the Efl.Net.Dialer interface.
 *
 * @param o The Efl_Net_Dialer_Simple object.
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @param klass The Efl_Class for the inner dialer.
 */
EOLIAN static void
_efl_net_dialer_simple_inner_class_set(Eo *o, Efl_Net_Dialer_Simple_Data *pd, const Efl_Class *klass)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   EINA_SAFETY_ON_NULL_RETURN(klass);
   pd->inner_class = klass;
   DBG("%p inner_class=%p %s", o, klass, efl_class_name_get(klass));
}

/**
 * @brief Gets the Efl_Class used for creating the inner dialer object.
 *
 * @param o The Efl_Net_Dialer_Simple object (unused).
 * @param pd The private data of the Efl_Net_Dialer_Simple object.
 * @return The Efl_Class for the inner dialer, or NULL if not set.
 */
EOLIAN static const Efl_Class *
_efl_net_dialer_simple_inner_class_get(const Eo *o EINA_UNUSED, Efl_Net_Dialer_Simple_Data *pd)
{
   return pd->inner_class;
}

#include "efl_net_dialer_simple.eo.c"
