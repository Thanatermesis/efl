#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define EFL_BETA_API_SUPPORT 1
#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Con.h>
#include "../eeze_scanner/eeze_scanner.h"

/**
 * @brief Eet data descriptor for Eeze_Scanner_Event.
 * Used for serializing and deserializing scanner events.
 */
static Eet_Data_Descriptor *edd;

/**
 * @brief Global return value for the application.
 * Modified in case of errors to indicate failure.
 */
static int retval = EXIT_SUCCESS;

/**
 * @brief Callback function to handle data received via Eet_Connection.
 * Decodes the received Eeze_Scanner_Event and prints its details.
 *
 * @param eet_data Pointer to the raw Eet data.
 * @param size Size of the Eet data.
 * @param user_data User data (unused in this context).
 * @return EINA_TRUE on successful processing, EINA_FALSE on error.
 */
static Eina_Bool
_eet_read(const void *eet_data, size_t size, void *user_data EINA_UNUSED)
{
   Eeze_Scanner_Event *ev = eet_data_descriptor_decode(edd, eet_data, size);

   if (!ev)
     {
        fprintf(stderr, "ERROR: could not decode event!\n");
        goto error;
     }

   switch (ev->type)
     {
      case EEZE_SCANNER_EVENT_TYPE_ADD:
         printf("ADD ");
         break;
      case EEZE_SCANNER_EVENT_TYPE_REMOVE:
         printf("DEL ");
         break;
      case EEZE_SCANNER_EVENT_TYPE_CHANGE:
         printf("CHG ");
         break;
      default:
         fprintf(stderr, "ERROR: unknown event type %d\n", ev->type);
         goto error;
     }

   if (ev->volume)
     printf("VOLUME ");

   printf("'%s'\n", ev->device);
   free(ev);

   return EINA_TRUE;

 error:
   retval = EXIT_FAILURE;
   ecore_main_loop_quit();
   return EINA_FALSE;
}

/**
 * @brief Callback function to handle data writing attempts via Eet_Connection.
 * This monitor is read-only, so any attempt to write is an error.
 *
 * @param eet_data Pointer to the Eet data to write (unused).
 * @param size Size of the Eet data (unused).
 * @param user_data User data (unused).
 * @return EINA_FALSE always, as writing is not supported.
 */
static Eina_Bool
_eet_write(const void *eet_data EINA_UNUSED, size_t size EINA_UNUSED, void *user_data EINA_UNUSED)
{
   fprintf(stderr, "ERROR: should not write data!\n");
   retval = EXIT_FAILURE;
   ecore_main_loop_quit();
   return EINA_FALSE;
}

/**
 * @brief Callback for the EFL_IO_BUFFERED_STREAM_EVENT_SLICE_CHANGED event.
 * Triggered when new data is available on the buffered stream.
 * It passes the received data to the Eet_Connection for processing.
 *
 * @param data The Eet_Connection instance.
 * @param event The Efl_Event details.
 */
static void
_on_data(void *data, const Efl_Event *event)
{
   Eet_Connection *ec = data;
   Eo *dialer = event->object;
   Eina_Slice slice;

   slice = efl_io_buffered_stream_slice_get(dialer);
   if (slice.len == 0) return;

   if (eet_connection_received(ec, slice.mem, slice.len) != 0)
     {
        fprintf(stderr, "ERROR: received invalid data\n");
        goto error;
     }

   efl_io_buffered_stream_discard(dialer, slice.len);

   return;

 error:
   retval = EXIT_FAILURE;
   ecore_main_loop_quit();
}

/**
 * @brief Callback for the EFL_IO_BUFFERED_STREAM_EVENT_READ_FINISHED event.
 * Triggered when the read stream has finished (e.g., connection closed by peer).
 * Quits the main loop.
 *
 * @param data User data (unused).
 * @param event The Efl_Event details (unused).
 */
static void
_finished(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   ecore_main_loop_quit();
}

/**
 * @brief Callback for the EFL_IO_BUFFERED_STREAM_EVENT_ERROR event.
 * Triggered when an error occurs on the stream.
 * Prints an error message and quits the main loop.
 *
 * @param data User data (unused).
 * @param event The Efl_Event details, containing the error information.
 */
static void
_error(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *dialer = event->object;
   Eina_Error *perr = event->info;

   fprintf(stderr, "ERROR: error communicating to %s: %s\n",
           efl_net_dialer_address_dial_get(dialer),
           eina_error_msg_get(*perr));
   retval = EXIT_FAILURE;
   ecore_main_loop_quit();
}

/**
 * @brief Main function for the eeze_scanner_monitor.
 * Initializes Ecore, Eet, Ecore_Con, sets up an Eet_Connection
 * to listen for eeze_scanner events, and enters the Ecore main loop.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */
int
main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Eet_Data_Descriptor_Class eddc;
   Eo *loop;
   char *path;
   Eo *dialer;
   Eet_Connection *ec;
   Eina_Error err;

   ecore_app_no_system_modules();

   eina_init();
   ecore_init();
   eet_init();
   ecore_con_init();

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), "eeze_scanner_event", sizeof(Eeze_Scanner_Event)))
     {
        fprintf(stderr, "ERROR: could not create eet data descriptor!\n");
        retval = EXIT_FAILURE;
        goto error_edd;
     }
   edd = eet_data_descriptor_stream_new(&eddc);
   if (!edd)
     {
        fprintf(stderr, "ERROR: could not create eet data descriptor!\n");
        retval = EXIT_FAILURE;
        goto error_edd;
     }
#define DAT(MEMBER, TYPE) EET_DATA_DESCRIPTOR_ADD_BASIC(edd, Eeze_Scanner_Event, #MEMBER, MEMBER, EET_T_##TYPE)
   DAT(device, INLINED_STRING);
   DAT(type, UINT);
   DAT(volume, UCHAR);
#undef DAT

   path = ecore_con_local_path_new(EINA_TRUE, "eeze_scanner", 0);
   if (!path)
     {
        fprintf(stderr, "ERROR: could not get local communication path\n");
        retval = EXIT_FAILURE;
        goto error_path;
     }

   loop = efl_main_loop_get();

#ifdef EFL_NET_DIALER_UNIX_CLASS
   dialer = efl_add(EFL_NET_DIALER_SIMPLE_CLASS, loop,
                    efl_net_dialer_simple_inner_class_set(efl_added, EFL_NET_DIALER_UNIX_CLASS));
#elif defined(EFL_NET_DIALER_WINDOWS_CLASS)
   dialer = efl_add(EFL_NET_DIALER_SIMPLE_CLASS, loop,
                    efl_net_dialer_simple_inner_class_set(efl_added, EFL_NET_DIALER_WINDOWS_CLASS));
#else
   fprintf(stderr, "ERROR: your platform doesn't support Efl.Net.Dialer.*\n");
#endif
   if (!dialer)
     {
        fprintf(stderr, "ERROR: could not create communication dialer\n");
        retval = EXIT_FAILURE;
        goto end;
     }

   ec = eet_connection_new(_eet_read, _eet_write, dialer);
   if (!ec)
     {
        fprintf(stderr, "ERROR: could not create Eet_Connection\n");
        retval = EXIT_FAILURE;
        goto end;
     }
   efl_event_callback_add(dialer, EFL_IO_BUFFERED_STREAM_EVENT_SLICE_CHANGED, _on_data, ec);


   efl_event_callback_add(dialer, EFL_IO_BUFFERED_STREAM_EVENT_ERROR, _error, NULL);
   efl_event_callback_add(dialer, EFL_IO_BUFFERED_STREAM_EVENT_READ_FINISHED, _finished, NULL);

   err = efl_net_dialer_dial(dialer, path);
   if (err)
     {
        fprintf(stderr, "ERROR: could not connect '%s': %s\n", path, eina_error_msg_get(err));
        retval = EXIT_FAILURE;
        goto end_dial;
     }

   ecore_main_loop_begin();

 end_dial:
   if (!efl_io_closer_closed_get(dialer))
     efl_io_closer_close(dialer);

   efl_event_callback_del(dialer, EFL_IO_BUFFERED_STREAM_EVENT_SLICE_CHANGED, _on_data, ec);
   eet_connection_close(ec, NULL);
   ec = NULL;

 end:
   efl_del(dialer);
   dialer = NULL;

   free(path);
   path = NULL;
 error_path:
   eet_data_descriptor_free(edd);
   edd = NULL;

 error_edd:
   ecore_con_shutdown();
   eet_shutdown();
   ecore_shutdown();
   eina_shutdown();
   return retval;
}
