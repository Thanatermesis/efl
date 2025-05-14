#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"
#include "Ecore_Evas.h"
#include "ecore_evas_private.h"
#include <Efl_Core.h>

/**
 * @internal
 * @brief Structure to hold fallback selection data for each buffer.
 *
 * This structure stores the callbacks and seat information associated with
 * a specific selection buffer when the primary selection mechanism is not
 * available or fails.
 */
typedef struct {
   Ecore_Evas_Selection_Callbacks callbacks[ECORE_EVAS_SELECTION_BUFFER_LAST]; /**< Array of callbacks for different selection buffers (e.g., PRIMARY, CLIPBOARD). */
   int seat; /**< The seat identifier associated with this selection. */
} Ecore_Evas_Fallback_Selection_Data;

/**
 * @internal
 * @brief Global array storing fallback selection data for all possible buffers.
 *
 * This array holds an Ecore_Evas_Fallback_Selection_Data instance for each
 * selection buffer type defined in ECORE_EVAS_SELECTION_BUFFER_LAST.
 */
static Ecore_Evas_Fallback_Selection_Data data[ECORE_EVAS_SELECTION_BUFFER_LAST];

/**
 * @internal
 * @brief Shuts down the fallback selection mechanism for a given Ecore_Evas.
 *
 * This function iterates through all registered fallback selection buffers
 * and calls their respective cancel callbacks if they exist. This is typically
 * called when an Ecore_Evas instance is being destroyed or the selection
 * mechanism is being reset.
 *
 * @param ee The Ecore_Evas instance for which to shut down fallback selection.
 */
void
fallback_selection_shutdown(Ecore_Evas *ee)
{
   for (int i = 0; i < ECORE_EVAS_SELECTION_BUFFER_LAST; ++i)
     {
        if (data->callbacks[i].cancel)
          data->callbacks[i].cancel(ee, data->seat, i);
     }
}

/**
 * @internal
 * @brief Claims ownership of a selection buffer in the fallback mechanism.
 *
 * This function is called when an application wants to provide data for a
 * specific selection buffer (e.g., PRIMARY, CLIPBOARD) using the fallback
 * mechanism. It registers the necessary callbacks for data delivery and
 * cancellation.
 *
 * If another part of the application previously claimed this selection buffer
 * via the fallback mechanism, its cancel callback will be invoked first.
 *
 * @param ee The Ecore_Evas instance claiming the selection.
 * @param seat The seat identifier for this claim.
 * @param selection The specific selection buffer to claim (e.g., ECORE_EVAS_SELECTION_BUFFER_PRIMARY).
 * @param available_types An Eina_Array of Eina_Stringshare, representing the MIME types
 *                        the claimant can provide for this selection.
 *                        Example:
 *                        @code
 *                        Eina_Array *types = eina_array_new(2);
 *                        eina_array_push(types, eina_stringshare_add("text/plain;charset=utf-8"));
 *                        eina_array_push(types, eina_stringshare_add("application/x-custom-type"));
 *                        @endcode
 * @param delivery The callback function to be invoked when a client requests data for this selection.
 * @param cancel The callback function to be invoked if this claim is superseded or cancelled.
 * @return EINA_TRUE on success, EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
Eina_Bool
fallback_selection_claim(Ecore_Evas *ee, unsigned int seat, Ecore_Evas_Selection_Buffer selection, Eina_Array *available_types, Ecore_Evas_Selection_Internal_Delivery delivery, Ecore_Evas_Selection_Internal_Cancel cancel)
{
   Ecore_Evas_Selection_Callbacks *callbacks = &data->callbacks[selection];

   if (callbacks->cancel)
     {
        callbacks->cancel(ee, data->seat, selection);
        eina_array_free(callbacks->available_types);
     }

   callbacks->delivery = delivery;
   callbacks->cancel = cancel;
   callbacks->available_types = available_types;
   data->seat = seat;

   if (ee->func.fn_selection_changed)
     ee->func.fn_selection_changed(ee, seat, selection);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Checks if a selection buffer has an owner in the fallback mechanism.
 *
 * This function is intended to determine if any application component has
 * claimed the specified selection buffer using the fallback mechanism.
 * However, the current implementation always returns EINA_FALSE, implying
 * that ownership status is primarily managed by the "real" (non-fallback)
 * selection system.
 *
 * @param ee The Ecore_Evas instance (currently unused).
 * @param seat The seat identifier (currently unused).
 * @param selection The selection buffer to check (currently unused).
 * @return EINA_FALSE, indicating the fallback mechanism doesn't independently assert ownership.
 */
Eina_Bool
fallback_selection_has_owner(Ecore_Evas *ee EINA_UNUSED, unsigned int seat EINA_UNUSED, Ecore_Evas_Selection_Buffer selection EINA_UNUSED)
{
   return EINA_FALSE; //if the real selection buffer does not contain it, then we dont know it either.
}

/**
 * @internal
 * @brief Finds the first mutually supported MIME type between acceptable and available types.
 *
 * This helper function iterates through the `available_types` and checks if any
 * of them are present in the `acceptable_types` list. It returns the first
 * matching type found. The `acceptable_types` array is freed by this function.
 *
 * @param acceptable_types An Eina_Array of Eina_Stringshare, representing MIME types
 *                         that the requester can understand. This array will be freed.
 *                         Example:
 *                         @code
 *                         Eina_Array *req_types = eina_array_new(1);
 *                         eina_array_push(req_types, eina_stringshare_add("text/plain;charset=utf-8"));
 *                         @endcode
 * @param available_types An Eina_Array of Eina_Stringshare, representing MIME types
 *                        that the provider can offer.
 *                        Example:
 *                        @code
 *                        Eina_Array *prov_types = eina_array_new(2);
 *                        eina_array_push(prov_types, eina_stringshare_add("text/uri-list"));
 *                        eina_array_push(prov_types, eina_stringshare_add("text/plain;charset=utf-8"));
 *                        @endcode
 * @return A new Eina_Stringshare reference to the first matching MIME type,
 *         or NULL if no common type is found. The caller is responsible for
 *         calling eina_stringshare_del() on the returned stringshare.
 */
Eina_Stringshare*
available_types(Eina_Array *acceptable_types, Eina_Array *available_types)
{
   Eina_Stringshare *found_type = NULL;
   Eina_Stringshare *type;

   for (unsigned int i = 0; i < eina_array_count_get(available_types); ++i)
     {
        unsigned int out = -1;
        type = eina_array_data_get(available_types, i);

        if (!found_type && eina_array_find(acceptable_types, type, &out))
          {
             found_type = eina_stringshare_ref(type);
          }
        eina_stringshare_del(type);
     }
  eina_array_free(acceptable_types);

  return found_type;
}

/**
 * @internal
 * @brief Requests data from a selection buffer using the fallback mechanism.
 *
 * This function is called when an application wants to retrieve data from a
 * specific selection buffer (e.g., PRIMARY, CLIPBOARD) that is managed by
 * the fallback mechanism. It identifies a mutually supported MIME type and
 * invokes the registered delivery callback to get the data.
 *
 * @param ee The Ecore_Evas instance requesting the selection (currently unused, but passed to delivery).
 * @param seat The seat identifier for this request.
 * @param selection The specific selection buffer to request data from.
 * @param acceptable_type An Eina_Array of Eina_Stringshare, representing the MIME types
 *                        the requester can understand. This array will be freed by
 *                        the `available_types` helper function.
 *                        Example:
 *                        @code
 *                        Eina_Array *types = eina_array_new(1);
 *                        eina_array_push(types, eina_stringshare_add("text/plain;charset=utf-8"));
 *                        @endcode
 * @return An Eina_Future that will resolve with an Eina_Value containing the
 *         selection data as an Eina_Content. The Eina_Value will hold an integer 0
 *         if no delivery callback is registered. Returns NULL if no suitable
 *         MIME type is found or if the delivery callback fails.
 *         The resolved Eina_Value (if successful and not int 0) will be of type
 *         EINA_VALUE_TYPE_CONTENT, where the Eina_Content encapsulates the data
 *         and its MIME type.
 */
Eina_Future*
fallback_selection_request(Ecore_Evas *ee EINA_UNUSED, unsigned int seat, Ecore_Evas_Selection_Buffer selection, Eina_Array *acceptable_type)
{
   Ecore_Evas_Selection_Callbacks callbacks = data->callbacks[selection];
   Eina_Content *result;
   Eina_Stringshare *serving_type;
   Eina_Rw_Slice slice_data;
   Eina_Value value;

   if (!callbacks.delivery)
     return eina_future_resolved(efl_loop_future_scheduler_get(efl_main_loop_get()), eina_value_int_init(0));

   serving_type = available_types(acceptable_type, callbacks.available_types);
   if (!serving_type)
     return NULL; //Silent return cause we cannot deliver a good type

   EINA_SAFETY_ON_FALSE_RETURN_VAL(callbacks.delivery(ee, seat, selection, serving_type, &slice_data), NULL);
   result = eina_content_new(eina_rw_slice_slice_get(slice_data), serving_type);
   value = eina_value_content_init(result);
   eina_content_free(result);

   return eina_future_resolved(efl_loop_future_scheduler_get(efl_main_loop_get()), value);
}

/**
 * @internal
 * @brief Starts a drag-and-drop operation using the fallback mechanism.
 *
 * This function is intended to initiate a DND operation when the primary
 * DND system is unavailable. However, the current implementation is a stub
 * and always returns EINA_FALSE, indicating that fallback DND is not
 * supported.
 *
 * @param ee The Ecore_Evas instance initiating the DND (currently unused).
 * @param seat The seat identifier for the DND operation (currently unused).
 * @param available_types An Eina_Array of Eina_Stringshare, representing the MIME types
 *                        offered for dragging (currently unused).
 * @param drag_rep The Ecore_Evas to use as the drag representation (e.g., a window showing the dragged item) (currently unused).
 * @param delivery The callback for data delivery (currently unused).
 * @param cancel The callback for DND cancellation (currently unused).
 * @param action The DND action string (e.g., "copy", "move", "link") (currently unused).
 * @return EINA_FALSE, as fallback DND is not implemented.
 */
Eina_Bool
fallback_dnd_start(Ecore_Evas *ee EINA_UNUSED, unsigned int seat EINA_UNUSED, Eina_Array *available_types EINA_UNUSED, Ecore_Evas *drag_rep EINA_UNUSED, Ecore_Evas_Selection_Internal_Delivery delivery EINA_UNUSED, Ecore_Evas_Selection_Internal_Cancel cancel EINA_UNUSED, const char* action EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stops an ongoing drag-and-drop operation in the fallback mechanism.
 *
 * This function is intended to terminate a DND operation initiated via the
 * fallback mechanism. As with `fallback_dnd_start`, this is currently a stub
 * and always returns EINA_FALSE.
 *
 * @param ee The Ecore_Evas instance related to the DND operation (currently unused).
 * @param seat The seat identifier for the DND operation (currently unused).
 * @return EINA_FALSE, as fallback DND is not implemented.
 */
Eina_Bool
fallback_dnd_stop(Ecore_Evas *ee EINA_UNUSED, unsigned int seat EINA_UNUSED)
{
   return EINA_FALSE;
}
