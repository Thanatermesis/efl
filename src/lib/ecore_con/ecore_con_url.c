/**
 * @file
 * @brief Ecore_Con_Url provides helper functions for network connections using URLs.
 *
 * This module leverages libcurl for handling URL-based network operations,
 * providing an abstraction layer over Efl.Net.Dialer.Http for legacy
 * Ecore_Con applications. It supports features like GET, POST, HEAD requests,
 * cookie management, proxy settings, SSL verification, and more.
 *
 * For info on how to use libcurl, see:
 * http://curl.haxx.se/libcurl/c/libcurl-tutorial.html
 *
 * FIXME: Support more CURL features...
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

#include "Ecore.h"
#include "ecore_private.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "ecore_con_url_curl.h"
#include "Emile.h"

/** Event type for URL data reception.
 * @ingroup Ecore_Con_Url_Group
 */
int ECORE_CON_EVENT_URL_DATA = 0;
/** Event type for URL connection completion.
 * @ingroup Ecore_Con_Url_Group
 */
int ECORE_CON_EVENT_URL_COMPLETE = 0;
/** Event type for URL connection progress.
 * @ingroup Ecore_Con_Url_Group
 */
int ECORE_CON_EVENT_URL_PROGRESS = 0;

static int _init_count = 0; /**< Counter for ecore_con_url_init() calls. */
static Eina_Bool pipelining = EINA_FALSE; /**< Flag indicating if HTTP pipelining is enabled. */

static Eina_List *_url_con_url_list = NULL; /**< List of active Ecore_Con_Url handles. */

/**
 * @addtogroup Ecore_Con_Url_Group Ecore URL Connection Functions
 *
 * @{
 */

/**
 * @brief Initializes the Ecore_Con_Url library.
 *
 * This function initializes all necessary subsystems for Ecore_Con_Url,
 * including Ecore, Ecore_Con, and Emile. It also registers new event types
 * for URL operations. This function increments an internal counter, and only
 * performs full initialization on the first call.
 *
 * @return The new initialization count. Returns 0 on failure.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API int
ecore_con_url_init(void)
{
   if (++_init_count > 1) return _init_count;
   if (!ecore_init()) goto ecore_init_failed;
   if (!ecore_con_init()) goto ecore_con_init_failed;
   if (!emile_init()) goto emile_init_failed;
   if (!emile_cipher_init()) goto emile_cipher_init_failed;
   ECORE_CON_EVENT_URL_DATA = ecore_event_type_new();
   ECORE_CON_EVENT_URL_COMPLETE = ecore_event_type_new();
   ECORE_CON_EVENT_URL_PROGRESS = ecore_event_type_new();
   return _init_count;

 emile_cipher_init_failed:
   emile_shutdown();
 emile_init_failed:
   ecore_con_shutdown();
 ecore_con_init_failed:
   ecore_shutdown();
 ecore_init_failed:
   return --_init_count;
}

/**
 * @brief Shuts down the Ecore_Con_Url library.
 *
 * This function shuts down all subsystems initialized by ecore_con_url_init().
 * It decrements an internal counter and only performs full shutdown when the
 * count reaches zero. It also frees any remaining Ecore_Con_Url handles.
 *
 * @return The new initialization count. Returns 0 if fully shut down.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API int
ecore_con_url_shutdown(void)
{
   Ecore_Con_Url *url_con_url;
   if (_init_count == 0) return 0;
   --_init_count;
   if (_init_count) return _init_count;
   EINA_LIST_FREE(_url_con_url_list, url_con_url)
     ecore_con_url_free(url_con_url);

   ecore_event_type_flush(ECORE_CON_EVENT_URL_DATA,
                          ECORE_CON_EVENT_URL_COMPLETE,
                          ECORE_CON_EVENT_URL_PROGRESS);

   emile_shutdown(); /* no emile_cipher_shutdown(), handled here */
   ecore_con_shutdown();
   ecore_shutdown();
   return 0;
}

/**
 * @brief Enables or disables HTTP pipelining.
 *
 * Pipelining allows sending multiple HTTP requests on the same persistent
 * connection without waiting for the corresponding responses.
 * This function requires the underlying curl library to be initialized.
 *
 * @param enable EINA_TRUE to enable pipelining, EINA_FALSE to disable.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_pipeline_set(Eina_Bool enable)
{
   if (!_c_init()) return;
   if (enable == pipelining) return;
   _c->curl_multi_setopt(_c->_curlm, CURLMOPT_PIPELINING, !!enable);
   pipelining = enable;
}

/**
 * @brief Gets the current state of HTTP pipelining.
 *
 * @return EINA_TRUE if pipelining is enabled, EINA_FALSE otherwise.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_pipeline_get(void)
{
   return pipelining;
}


/* The rest of this file exists solely to provide ABI compatibility */

/**
 * @brief Structure representing an Ecore_Con_Url connection handle.
 * @typedef Ecore_Con_Url
 * @ingroup Ecore_Con_Url_Group
 *
 * This structure holds all the state for a single URL connection.
 * It is an opaque type, and its members should not be accessed directly.
 */
struct _Ecore_Con_Url
{
   ECORE_MAGIC; /**< Magic number for type checking. */
   Eo *dialer; /**< The underlying Efl_Net_Dialer_Http object. */
   Eo *send_copier; /**< Efl_Io_Copier for POST/PUT data. */
   Eo *input; /**< Input source for POST/PUT data (e.g., Efl_Io_Buffer or Efl_Io_File). */
   Ecore_Timer *timer; /**< Timeout timer for the connection. */
   struct {
      Ecore_Animator *animator; /**< Animator for progress updates. */
      struct {
         uint64_t total; /**< Total bytes expected for download. */
         uint64_t now;   /**< Current bytes downloaded. */
      } download, upload; /**< Download and upload progress information. */
   } progress;
   Eina_Stringshare *url; /**< The URL for the connection. */
   Eina_Stringshare *custom_request; /**< Custom HTTP request method (e.g., "PUT", "DELETE"). */
   void *data; /**< User-specific data associated with this handle. */
   struct {
      Eina_List *files; /**< List of Eina_Stringshare: file paths to read cookies from. */
      Eina_List *cmds;  /**< List of static const char*: COOKIELIST commands (e.g., "ALL", "SESS"). */
      Eina_Stringshare *jar; /**< Eina_Stringshare: file path to write cookies to (cookie jar). */
      Eina_Bool ignore_old_session; /**< If EINA_TRUE, ignore session cookies from previous sessions. */
   } cookies;
   struct {
      Eina_Stringshare *url; /**< Proxy server URL. */
      Eina_Stringshare *username; /**< Username for proxy authentication. */
      Eina_Stringshare *password; /**< Password for proxy authentication. */
   } proxy;
   struct {
      Ecore_Con_Url_Time condition; /**< Time condition for the request (e.g., If-Modified-Since). */
      double stamp; /**< Timestamp for the time condition. */
   } time;
   struct {
      Eina_Stringshare *username; /**< Username for HTTP authentication. */
      Eina_Stringshare *password; /**< Password for HTTP authentication. */
      Efl_Net_Http_Authentication_Method method; /**< HTTP authentication method. */
      Eina_Bool restricted; /**< If EINA_TRUE, use only "safe" authentication methods. */
   } httpauth;
   Eina_Stringshare *ca_path; /**< Path to CA certificate(s) for SSL verification. */
   Eina_List *request_headers; /**< List of Efl_Net_Http_Header: custom headers to send with the request. */
   Eina_List *response_headers; /**< List of char*: headers received from the server. Each string is a full header line. */
   unsigned event_count; /**< Count of pending events associated with this handle. */
   int received_bytes; /**< Total bytes received in the current transfer. */
   int status; /**< HTTP status code of the response. Set when complete. */
   int write_fd; /**< File descriptor to write received data to. -1 if not used. */
   Efl_Net_Http_Version http_version; /**< HTTP protocol version to use. */
   Eina_Bool ssl_verify_peer; /**< If EINA_TRUE, verify the SSL peer certificate. */
   Eina_Bool verbose; /**< If EINA_TRUE, enable verbose output for debugging. */
   Eina_Bool ftp_use_epsv; /**< If EINA_TRUE, use EPSV for FTP transfers. */
   Eina_Bool delete_me; /**< If EINA_TRUE, the handle is marked for deletion. */
};

#define ECORE_CON_URL_CHECK_RETURN(u, ...) \
  do \
    { \
       if (!EINA_MAGIC_CHECK(u, ECORE_MAGIC_CON_URL)) \
         { \
            ECORE_MAGIC_FAIL(u, ECORE_MAGIC_CON_URL, __func__); \
            return __VA_ARGS__; \
         } \
       EINA_SAFETY_ON_TRUE_RETURN_VAL(u->delete_me, __VA_ARGS__); \
    } \
  while (0)


/**
 * @internal
 * @brief Closes and cleans up resources associated with the dialer for a given Ecore_Con_Url.
 *
 * This function handles the deletion of the send copier, input source, timer,
 * progress animator, and the dialer itself. It ensures that the dialer is closed
 * if it's still open.
 *
 * @param url_con The Ecore_Con_Url handle.
 */
static void
_ecore_con_url_dialer_close(Ecore_Con_Url *url_con)
{
   if (url_con->send_copier)
     {
        efl_del(url_con->send_copier);
        url_con->send_copier = NULL;
     }

   if (url_con->input)
     {
        efl_del(url_con->input);
        url_con->input = NULL;
     }

   if (url_con->timer)
     {
        ecore_timer_del(url_con->timer);
        url_con->timer = NULL;
     }

   if (url_con->progress.animator)
     {
        ecore_animator_del(url_con->progress.animator);
        url_con->progress.animator = NULL;
     }

   if (!url_con->dialer) return;

   if (!efl_io_closer_closed_get(url_con->dialer))
     efl_io_closer_close(url_con->dialer);
   efl_del(url_con->dialer);
   url_con->dialer = NULL;
}

/**
 * @internal
 * @brief Frees the memory allocated for storing response headers.
 *
 * Iterates through the list of response headers and frees each header string.
 *
 * @param url_con The Ecore_Con_Url handle whose response headers are to be freed.
 */
static void
_ecore_con_url_response_headers_free(Ecore_Con_Url *url_con)
{
   char *str;
   EINA_LIST_FREE(url_con->response_headers, str)
     free(str);
}

/**
 * @internal
 * @brief Frees the memory allocated for storing request headers.
 *
 * Iterates through the list of request headers (Efl_Net_Http_Header structures)
 * and frees each one. The key and value strings are assumed to be part of
 * the same allocation as the header structure itself.
 *
 * @param url_con The Ecore_Con_Url handle whose request headers are to be freed.
 */
static void
_ecore_con_url_request_headers_free(Ecore_Con_Url *url_con)
{
   Efl_Net_Http_Header *header;
   EINA_LIST_FREE(url_con->response_headers, header)
     free(header); /* key and value are inline */
}

/**
 * @internal
 * @brief Internal function to free an Ecore_Con_Url handle and its associated resources.
 *
 * This function is called when an Ecore_Con_Url handle is no longer needed and
 * has no pending events. It cleans up all allocated resources, including stringshares,
 * lists, and the dialer.
 *
 * @param url_con The Ecore_Con_Url handle to free.
 */
static void
_ecore_con_url_free_internal(Ecore_Con_Url *url_con)
{
   const char *s;

   url_con->delete_me = EINA_TRUE;
   if (url_con->event_count > 0) return;

   _ecore_con_url_dialer_close(url_con);

   eina_stringshare_replace(&url_con->url, NULL);
   eina_stringshare_replace(&url_con->custom_request, NULL);

   url_con->data = NULL;

   EINA_LIST_FREE(url_con->cookies.files, s)
     eina_stringshare_del(s);
   eina_list_free(url_con->cookies.cmds); /* data is not to be freed! */
   eina_stringshare_replace(&url_con->cookies.jar, NULL);

   eina_stringshare_replace(&url_con->proxy.url, NULL);
   eina_stringshare_replace(&url_con->proxy.username, NULL);
   eina_stringshare_replace(&url_con->proxy.password, NULL);

   eina_stringshare_replace(&url_con->httpauth.username, NULL);
   eina_stringshare_replace(&url_con->httpauth.password, NULL);

   eina_stringshare_replace(&url_con->ca_path, NULL);

   _ecore_con_url_request_headers_free(url_con);
   _ecore_con_url_response_headers_free(url_con);

   ECORE_MAGIC_SET(url_con, ECORE_MAGIC_NONE);
   free(url_con);
}

/**
 * @internal
 * @brief Frees an Ecore_Con_Event_Url_Progress event structure.
 *
 * This is a callback function used by the Ecore event system. It decrements
 * the event count on the associated Ecore_Con_Url handle and, if the handle
 * is marked for deletion and has no more pending events, frees the handle.
 *
 * @param data Unused user data.
 * @param event The Ecore_Con_Event_Url_Progress event to free.
 */
static void
_ecore_con_event_url_progress_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Progress *ev = event;
   Ecore_Con_Url *url_con = ev->url_con;

   EINA_SAFETY_ON_TRUE_GOTO(url_con->event_count == 0, end);

   url_con->event_count--;
   if ((url_con->event_count == 0) && (url_con->delete_me))
     _ecore_con_url_free_internal(url_con);

 end:
   free(ev);
}

/**
 * @internal
 * @brief Creates and adds an ECORE_CON_EVENT_URL_PROGRESS event to the event queue.
 *
 * This function is called to notify listeners about the progress of a URL transfer.
 * It populates an Ecore_Con_Event_Url_Progress structure with the current
 * download and upload progress from the Ecore_Con_Url handle.
 *
 * @param url_con The Ecore_Con_Url handle for which to report progress.
 */
static void
_ecore_con_event_url_progress_add(Ecore_Con_Url *url_con)
{
   Ecore_Con_Event_Url_Progress *ev;

   if (url_con->delete_me) return;

   ev = malloc(sizeof(*ev));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->url_con = url_con;
   ev->down.total = url_con->progress.download.total;
   ev->down.now = url_con->progress.download.now;
   ev->up.total = url_con->progress.upload.total;
   ev->up.now = url_con->progress.upload.now;
   url_con->event_count++;
   ecore_event_add(ECORE_CON_EVENT_URL_PROGRESS, ev, _ecore_con_event_url_progress_free, NULL);
}

/**
 * @internal
 * @brief Frees an Ecore_Con_Event_Url_Complete event structure.
 *
 * This is a callback function used by the Ecore event system. It decrements
 * the event count on the associated Ecore_Con_Url handle and, if the handle
 * is marked for deletion and has no more pending events, frees the handle.
 *
 * @param data Unused user data.
 * @param event The Ecore_Con_Event_Url_Complete event to free.
 */
static void
_ecore_con_event_url_complete_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Complete *ev = event;
   Ecore_Con_Url *url_con = ev->url_con;

   EINA_SAFETY_ON_TRUE_GOTO(url_con->event_count == 0, end);

   url_con->event_count--;
   if ((url_con->event_count == 0) && (url_con->delete_me))
     _ecore_con_url_free_internal(url_con);

 end:
   free(ev);
}

/**
 * @internal
 * @brief Creates and adds an ECORE_CON_EVENT_URL_COMPLETE event to the event queue.
 *
 * This function is called when a URL transfer is complete (either successfully
 * or with an error). It cleans up progress reporting, sets the status on the
 * Ecore_Con_Url handle, and queues the completion event. It also closes
 * the dialer associated with the handle.
 *
 * @param url_con The Ecore_Con_Url handle that has completed.
 * @param status The completion status code (e.g., HTTP status code or an error code).
 */
static void
_ecore_con_event_url_complete_add(Ecore_Con_Url *url_con, int status)
{
   Ecore_Con_Event_Url_Complete *ev;

   if (url_con->delete_me) return;

   if (url_con->progress.animator)
     {
        ecore_animator_del(url_con->progress.animator);
        url_con->progress.animator = NULL;
        _ecore_con_event_url_progress_add(url_con); /* Send one last progress event */
     }

   if (url_con->status)
     {
        DBG("URL '%s' was already complete with status=%d, new=%d", url_con->url, url_con->status, status);
        goto end;
     }

   url_con->status = status;

   ev = malloc(sizeof(Ecore_Con_Event_Url_Complete));
   EINA_SAFETY_ON_NULL_GOTO(ev, end);

   ev->url_con = url_con;
   ev->status = status;
   url_con->event_count++;
   ecore_event_add(ECORE_CON_EVENT_URL_COMPLETE, ev, _ecore_con_event_url_complete_free, NULL);

 end:
   _ecore_con_url_dialer_close(url_con);
}

/**
 * @internal
 * @brief Callback for EFL_NET_DIALER_EVENT_DIALER_ERROR events from the HTTP dialer.
 *
 * This function is invoked when the underlying Efl_Net_Dialer_Http object
 * encounters an error. It logs the error and triggers a completion event
 * with an appropriate status code.
 *
 * @param data The Ecore_Con_Url handle associated with the dialer.
 * @param event The Efl_Event containing error information.
 */
static void
_ecore_con_url_dialer_error(void *data, const Efl_Event *event)
{
   Ecore_Con_Url *url_con = data;
   Eina_Error *perr = event->info;
   int status;

   status = efl_net_dialer_http_response_status_get(url_con->dialer);
   if ((status < 500) || (status > 599))
     {
        DBG("HTTP error %d reset to 1", status);
        status = 1; /* not a real HTTP error */
     }

   WRN("HTTP dialer error url='%s': %s",
       efl_net_dialer_address_dial_get(url_con->dialer),
       eina_error_msg_get(*perr));

   _ecore_con_event_url_complete_add(url_con, status);
}

/**
 * @internal
 * @brief Frees an Ecore_Con_Event_Url_Data event structure.
 *
 * This is a callback function used by the Ecore event system. It decrements
 * the event count on the associated Ecore_Con_Url handle and, if the handle
 * is marked for deletion and has no more pending events, frees the handle.
 *
 * @param data Unused user data.
 * @param event The Ecore_Con_Event_Url_Data event to free.
 */
static void
_ecore_con_event_url_data_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;
   Ecore_Con_Url *url_con = ev->url_con;

   EINA_SAFETY_ON_TRUE_GOTO(url_con->event_count == 0, end);

   url_con->event_count--;
   if ((url_con->event_count == 0) && (url_con->delete_me))
     _ecore_con_url_free_internal(url_con);

 end:
   free(ev);
}

/**
 * @internal
 * @brief Callback for EFL_IO_READER_EVENT_CAN_READ_CHANGED events from the HTTP dialer.
 *
 * This function is invoked when the underlying Efl_Net_Dialer_Http object
 * has data available to be read. It reads the data, and either sends an
 * ECORE_CON_EVENT_URL_DATA event or writes the data to the `write_fd`
 * if one is set.
 *
 * @param data The Ecore_Con_Url handle associated with the dialer.
 * @param event The Efl_Event (unused in this function).
 */
static void
_ecore_con_url_dialer_can_read_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Ecore_Con_Url *url_con = data;
   Eina_Bool can_read;
   Ecore_Con_Event_Url_Data *ev;
   Eina_Rw_Slice slice;
   Eina_Error err;

   if (url_con->delete_me) return;

   can_read = efl_io_reader_can_read_get(url_con->dialer);
   if (!can_read) return;

   ev = malloc(sizeof(Ecore_Con_Event_Url_Data) + EFL_NET_DIALER_HTTP_BUFFER_RECEIVE_SIZE);
   EINA_SAFETY_ON_NULL_RETURN(ev);

   slice.mem = ev->data;
   slice.len = EFL_NET_DIALER_HTTP_BUFFER_RECEIVE_SIZE;

   err = efl_io_reader_read(url_con->dialer, &slice);
   if (err)
     {
        free(ev);
        if (err == EAGAIN) return;
        WRN("Error reading data from HTTP url='%s': %s",
            efl_net_dialer_address_dial_get(url_con->dialer),
            eina_error_msg_get(err));
        return;
     }

   ev->size = slice.len;
   ev->url_con = url_con;
   url_con->received_bytes += ev->size;

   if (url_con->write_fd == -1)
     {
        url_con->event_count++;
        ecore_event_add(ECORE_CON_EVENT_URL_DATA, ev, _ecore_con_event_url_data_free, NULL);
        return;
     }

   while (slice.len > 0)
     {
        ssize_t r = write(url_con->write_fd, slice.bytes, slice.len);
        if (r == -1)
          {
             ERR("Could not write to fd=%d: %s", url_con->write_fd, eina_error_msg_get(errno));
             break;
          }
        slice.bytes += r;
        slice.len -= r;
     }
   free(ev);
}

/**
 * @internal
 * @brief Callback for EFL_IO_READER_EVENT_EOS (End Of Stream) events from the HTTP dialer.
 *
 * This function is invoked when the underlying Efl_Net_Dialer_Http object
 * signals that all data has been received. If there's no pending send operation,
 * it triggers a completion event.
 *
 * @param data The Ecore_Con_Url handle associated with the dialer.
 * @param event The Efl_Event (unused in this function).
 */
static void
_ecore_con_url_dialer_eos(void *data, const Efl_Event *event EINA_UNUSED)
{
   Ecore_Con_Url *url_con = data;

   DBG("HTTP EOS url='%s'", efl_net_dialer_address_dial_get(url_con->dialer));

   if (url_con->send_copier && (!efl_io_copier_done_get(url_con->send_copier)))
     {
        DBG("done receiving, waiting for send copier...");
        return;
     }

   _ecore_con_event_url_complete_add(url_con, efl_net_dialer_http_response_status_get(url_con->dialer));
}

/**
 * @internal
 * @brief Callback for EFL_NET_DIALER_HTTP_EVENT_HEADERS_DONE events from the HTTP dialer.
 *
 * This function is invoked when the underlying Efl_Net_Dialer_Http object
 * has finished receiving all response headers. It retrieves the headers,
 * formats them as strings, and stores them in the `response_headers` list
 * of the Ecore_Con_Url handle.
 *
 * @param data The Ecore_Con_Url handle associated with the dialer.
 * @param event The Efl_Event (unused in this function).
 */
static void
_ecore_con_url_dialer_headers_done(void *data, const Efl_Event *event EINA_UNUSED)
{
   Ecore_Con_Url *url_con = data;
   Eina_Iterator *it;
   Efl_Net_Http_Header *header;
   size_t len;
   int status = efl_net_dialer_http_response_status_get(url_con->dialer);
   char *str;

   DBG("HTTP headers done, status=%d url='%s'",
       status,
       efl_net_dialer_address_dial_get(url_con->dialer));

   _ecore_con_url_response_headers_free(url_con);

   it = efl_net_dialer_http_response_headers_all_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(it);
   EINA_ITERATOR_FOREACH(it, header)
     {
        if (header->key)
          {
             len = strlen(header->key) + strlen(header->value) + strlen(": \r\n") + 1;
             str = malloc(len);
             EINA_SAFETY_ON_NULL_GOTO(str, end);
             snprintf(str, len, "%s: %s\r\n", header->key, header->value);
             url_con->response_headers = eina_list_append(url_con->response_headers, str);
          }
        else
          {
             if (url_con->response_headers)
               {
                  str = malloc(strlen("\r\n") + 1);
                  EINA_SAFETY_ON_NULL_GOTO(str, end);
                  memcpy(str, "\r\n", strlen("\r\n") + 1);
                  url_con->response_headers = eina_list_append(url_con->response_headers, str);
               }

             len = strlen(header->value) + strlen("\r\n") + 1;
             str = malloc(len);
             EINA_SAFETY_ON_NULL_GOTO(str, end);
             snprintf(str, len, "%s\r\n", header->value);
             url_con->response_headers = eina_list_append(url_con->response_headers, str);
          }
     }

   str = malloc(strlen("\r\n") + 1);
   EINA_SAFETY_ON_NULL_GOTO(str, end);
   memcpy(str, "\r\n", strlen("\r\n") + 1);
   url_con->response_headers = eina_list_append(url_con->response_headers, str);

 end:
   eina_iterator_free(it);
}

EFL_CALLBACKS_ARRAY_DEFINE(ecore_con_url_dialer_cbs,
                           { EFL_IO_READER_EVENT_CAN_READ_CHANGED, _ecore_con_url_dialer_can_read_changed },
                           { EFL_IO_READER_EVENT_EOS, _ecore_con_url_dialer_eos },
                           { EFL_NET_DIALER_EVENT_DIALER_ERROR, _ecore_con_url_dialer_error },
                           { EFL_NET_DIALER_HTTP_EVENT_HEADERS_DONE, _ecore_con_url_dialer_headers_done });

/**
 * @internal
 * @brief Animator callback for periodically checking and reporting transfer progress.
 *
 * This function is called by an Ecore_Animator. It retrieves the current
 * download and upload progress from the dialer. If the progress has changed
 * since the last check, it updates the Ecore_Con_Url handle's progress
 * information and triggers an ECORE_CON_EVENT_URL_PROGRESS event.
 *
 * @param data The Ecore_Con_Url handle.
 * @return EINA_TRUE to continue the animator, EINA_FALSE to stop.
 */
static Eina_Bool
_ecore_con_url_progress_animator_cb(void *data)
{
   Ecore_Con_Url *url_con = data;
   uint64_t dn, dt, un, ut;

   efl_net_dialer_http_progress_download_get(url_con->dialer, &dn, &dt);
   efl_net_dialer_http_progress_upload_get(url_con->dialer, &un, &ut);

   if ((dn == url_con->progress.download.now) &&
       (dt == url_con->progress.download.total) &&
       (un == url_con->progress.upload.now) &&
       (ut == url_con->progress.upload.total))
     return EINA_TRUE;

   url_con->progress.download.now = dn;
   url_con->progress.download.total = dt;
   url_con->progress.upload.now = un;
   url_con->progress.upload.total = ut;

   _ecore_con_event_url_progress_add(url_con);

   return EINA_TRUE;
}

/*
 * creates a new efl_net_dialer_proxy_set() URL based on legacy parameters:
 *  - proxy url (that could contain user + pass encoded, optional protocol)
 *    - no protocol = http://
 *  - username
 *  - password
 *
 * This function constructs a full proxy URL string suitable for
 * `efl_net_dialer_proxy_set()` by combining the proxy URL, username,
 * and password fields from the `Ecore_Con_Url` structure. It handles
 * cases where parts of the proxy information might be embedded in the
 * proxy URL itself or provided separately.
 *
 * If `url_con->proxy.url` is NULL, this function returns NULL, indicating
 * that the proxy should be determined from environment variables (e.g., http_proxy).
 *
 * The returned string is dynamically allocated and must be freed by the caller.
 *
 * @param url_con The Ecore_Con_Url handle containing proxy settings.
 * @return A newly allocated string with the full proxy URL, or NULL.
 */
static char *
_ecore_con_url_proxy_url_new(const Ecore_Con_Url *url_con)
{
   Eina_Slice protocol, user = {}, pass = {}, address;
   char buf[4096];
   const char *p;

   if (!url_con->proxy.url) return NULL; /* use from envvar */

   p = strstr(url_con->proxy.url, "://");
   if (!p)
     {
        protocol = (Eina_Slice)EINA_SLICE_STR_LITERAL("http");
        address = (Eina_Slice)EINA_SLICE_STR(url_con->proxy.url);
     }
   else
     {
        const char *s;

        protocol.mem = url_con->proxy.url;
        protocol.len = p - url_con->proxy.url;
        if (protocol.len == 0)
          protocol = (Eina_Slice)EINA_SLICE_STR_LITERAL("http");

        p += strlen("://");
        s = strchr(p, '@');
        if (!s)
          {
             address = (Eina_Slice)EINA_SLICE_STR(p);
          }
        else
          {
             address = (Eina_Slice)EINA_SLICE_STR(s + 1);

             user.mem = p;
             user.len = s - p;

             s = eina_slice_strchr(user, ':');
             if (s)
               {
                  pass.mem = s + 1;
                  pass.len = user.len - (user.bytes - pass.bytes);
                  user.len = s - (const char *)user.bytes;
               }
          }
     }

   if (url_con->proxy.username)
     user = eina_stringshare_slice_get(url_con->proxy.username);

   if (url_con->proxy.password)
     pass = eina_stringshare_slice_get(url_con->proxy.password);

   if (user.len && pass.len)
     {
        snprintf(buf, sizeof(buf),
                 EINA_SLICE_STR_FMT "://"
                 EINA_SLICE_STR_FMT ":"
                 EINA_SLICE_STR_FMT "@"
                 EINA_SLICE_STR_FMT,
                 EINA_SLICE_STR_PRINT(protocol),
                 EINA_SLICE_STR_PRINT(user),
                 EINA_SLICE_STR_PRINT(pass),
                 EINA_SLICE_STR_PRINT(address));
     }
   else if (user.len)
     {
        snprintf(buf, sizeof(buf),
                 EINA_SLICE_STR_FMT "://"
                 EINA_SLICE_STR_FMT "@"
                 EINA_SLICE_STR_FMT,
                 EINA_SLICE_STR_PRINT(protocol),
                 EINA_SLICE_STR_PRINT(user),
                 EINA_SLICE_STR_PRINT(address));
     }
   else
     {
        snprintf(buf, sizeof(buf),
                 EINA_SLICE_STR_FMT "://" EINA_SLICE_STR_FMT,
                 EINA_SLICE_STR_PRINT(protocol),
                 EINA_SLICE_STR_PRINT(address));
     }

   return strdup(buf);
}

/**
 * @internal
 * @brief Callback for EFL_IO_COPIER_EVENT_DONE events from the send copier.
 *
 * This function is invoked when the Efl_Io_Copier used for sending data
 * (e.g., in a POST or PUT request) has finished its operation. If the
 * receiving side (dialer) has also reached EOS (End Of Stream), this
 * triggers a completion event for the URL transfer.
 *
 * @param data The Ecore_Con_Url handle associated with the copier.
 * @param event The Efl_Event from the copier.
 */
static void
_ecore_con_url_copier_done(void *data, const Efl_Event *event)
{
   Ecore_Con_Url *url_con = data;
   int status = efl_net_dialer_http_response_status_get(url_con->dialer);

   DBG("copier %s %p for url='%s' is done", efl_name_get(event->object), event->object, efl_net_dialer_address_dial_get(url_con->dialer));

   if (!efl_io_reader_eos_get(url_con->dialer))
     {
        DBG("done sending, waiting for dialer EOS...");
        return;
     }

   _ecore_con_event_url_complete_add(url_con, status);
}

/**
 * @internal
 * @brief Callback for EFL_IO_COPIER_EVENT_ERROR events from the send copier.
 *
 * This function is invoked when the Efl_Io_Copier used for sending data
 * encounters an error. It logs the error and triggers a completion event
 * for the URL transfer with an appropriate status code.
 *
 * @param data The Ecore_Con_Url handle associated with the copier.
 * @param event The Efl_Event containing error information from the copier.
 */
static void
_ecore_con_url_copier_error(void *data, const Efl_Event *event)
{
   Ecore_Con_Url *url_con = data;
   Eina_Error *perr = event->info;
   int status;

   status = efl_net_dialer_http_response_status_get(url_con->dialer);
   if ((status < 500) || (status > 599))
     {
        DBG("HTTP error %d reset to 1", status);
        status = 1; /* not a real HTTP error */
     }

   WRN("HTTP copier %s %p error url='%s': %s",
       efl_name_get(event->object), event->object,
       efl_net_dialer_address_dial_get(url_con->dialer),
       eina_error_msg_get(*perr));

   _ecore_con_event_url_complete_add(url_con, status);
}

EFL_CALLBACKS_ARRAY_DEFINE(_ecore_con_url_copier_cbs,
                           { EFL_IO_COPIER_EVENT_ERROR, _ecore_con_url_copier_error },
                           { EFL_IO_COPIER_EVENT_DONE, _ecore_con_url_copier_done });

/*
 * Ecore_Con_Url is documented as 'reusable', while Efl.Net.Dialers
 * are one-shot and must be recreated on every usage.
 *
 * This function, _ecore_con_url_request_prepare(), handles this by:
 * 1. Closing any existing dialer associated with `url_con`.
 * 2. Resetting various state variables in `url_con` (status, received_bytes, progress, response_headers).
 * 3. Constructing the proxy URL using `_ecore_con_url_proxy_url_new()`.
 * 4. Creating a new `EFL_NET_DIALER_HTTP_CLASS` instance.
 * 5. Configuring the new dialer with all relevant settings from `url_con`, such as:
 *    - HTTP method (GET, POST, custom).
 *    - Primary mode (upload/download).
 *    - Proxy settings.
 *    - HTTP authentication.
 *    - HTTP version.
 *    - Redirect policy.
 *    - SSL verification settings (peer verification, CA path).
 *    - Event callbacks.
 * 6. Accessing the underlying CURL easy handle to set CURL-specific options:
 *    - Verbose mode / debug function.
 *    - FTP EPSV mode.
 *    - Default "Accept-Encoding" header.
 *    - Time conditions (If-Modified-Since, If-Unmodified-Since).
 *    - Custom request headers.
 *    - Cookie settings (session behavior, cookie files, cookie jar, cookie commands).
 * 7. Starting an Ecore_Animator for progress updates if needed.
 *
 * @param url_con The Ecore_Con_Url handle to prepare for a new request.
 * @param method The HTTP method string (e.g., "GET", "POST", "HEAD").
 * @return EINA_TRUE if the preparation was successful and a new dialer is ready,
 *         EINA_FALSE on failure (e.g., memory allocation error, failed to create dialer).
 */
static Eina_Bool
_ecore_con_url_request_prepare(Ecore_Con_Url *url_con, const char *method)
{
   const Efl_Net_Http_Header *header;
   const Eina_List *n;
   const char *s;
   char *proxy_url = NULL;
   CURL *curl_easy;

   _ecore_con_url_dialer_close(url_con);

   url_con->status = 0;
   url_con->received_bytes = 0;
   url_con->progress.download.now = 0;
   url_con->progress.download.total = 0;
   url_con->progress.upload.now = 0;
   url_con->progress.upload.total = 0;
   _ecore_con_url_response_headers_free(url_con);

   proxy_url = _ecore_con_url_proxy_url_new(url_con);
   if (proxy_url)
     DBG("proxy_url='%s'", proxy_url);

   url_con->dialer = efl_add(EFL_NET_DIALER_HTTP_CLASS, efl_main_loop_get(),
                             efl_net_dialer_http_method_set(efl_added, url_con->custom_request ? url_con->custom_request : method),
                             efl_net_dialer_http_primary_mode_set(efl_added, (strcmp(method, "PUT") == 0) ? EFL_NET_DIALER_HTTP_PRIMARY_MODE_UPLOAD : EFL_NET_DIALER_HTTP_PRIMARY_MODE_DOWNLOAD),
                             efl_net_dialer_proxy_set(efl_added, proxy_url),
                             efl_net_dialer_http_authentication_set(efl_added, url_con->httpauth.username, url_con->httpauth.password, url_con->httpauth.method, url_con->httpauth.restricted),
                             efl_net_dialer_http_version_set(efl_added, url_con->http_version),
                             efl_net_dialer_http_allow_redirects_set(efl_added, EINA_TRUE),
                             efl_net_dialer_http_ssl_verify_set(efl_added, url_con->ssl_verify_peer, url_con->ssl_verify_peer),
                             efl_net_dialer_http_ssl_certificate_authority_set(efl_added, url_con->ca_path),
                             efl_event_callback_array_add(efl_added, ecore_con_url_dialer_cbs(), url_con));
   EINA_SAFETY_ON_NULL_GOTO(url_con->dialer, error);

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_GOTO(curl_easy, error_curl_easy);

   if (url_con->verbose)
     {
        _c->curl_easy_setopt(curl_easy, CURLOPT_DEBUGFUNCTION, NULL);
        _c->curl_easy_setopt(curl_easy, CURLOPT_DEBUGDATA, NULL);
        _c->curl_easy_setopt(curl_easy, CURLOPT_VERBOSE, 1L);
        DBG("HTTP Dialer %p is set to legacy debug function (CURL's default, no eina_log)", url_con->dialer);
     }

   _c->curl_easy_setopt(curl_easy, CURLOPT_FTP_USE_EPSV, (long)url_con->ftp_use_epsv);

   /* previously always set encoding to gzip,deflate */
   efl_net_dialer_http_request_header_add(url_con->dialer, "Accept-Encoding", "gzip,deflate");

   if (url_con->time.condition != ECORE_CON_URL_TIME_NONE)
     {
        char *ts = efl_net_dialer_http_date_serialize(url_con->time.stamp);
        if (ts)
          {
             efl_net_dialer_http_request_header_add(url_con->dialer,
                                                    url_con->time.condition == ECORE_CON_URL_TIME_IFMODSINCE ? "If-Modified-Since" : "If-Unmodified-Since",
                                                    ts);
             free(ts);
          }
     }

   EINA_LIST_FOREACH(url_con->request_headers, n, header)
     efl_net_dialer_http_request_header_add(url_con->dialer, header->key, header->value);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIESESSION, (long)url_con->cookies.ignore_old_session);

   EINA_LIST_FOREACH(url_con->cookies.files, n, s)
     _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIEFILE, s);

   if (url_con->cookies.jar)
     _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIEJAR, url_con->cookies.jar);

   EINA_LIST_FREE(url_con->cookies.cmds, s) /* free: only to execute once! */
     _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIELIST, s);

   // Users should hook to their window animator if they want to show in real-time,
   // or have a slower timer... but the old API requested a period event, so add it
   // based on global animator timeout
   url_con->progress.animator = ecore_animator_add(_ecore_con_url_progress_animator_cb, url_con);

   DBG("prepared %p %s (%s), proxy=%s, primary_mode=%d",
       url_con->dialer,
       method,
       efl_net_dialer_http_method_get(url_con->dialer),
       efl_net_dialer_proxy_get(url_con->dialer),
       efl_net_dialer_http_primary_mode_get(url_con->dialer));

   free(proxy_url);
   return EINA_TRUE;

 error_curl_easy:
   _ecore_con_url_dialer_close(url_con);
 error:
   free(proxy_url);
   return EINA_FALSE;
}

/**
 * @brief Creates a new Ecore_Con_Url handle.
 *
 * Initializes a new Ecore_Con_Url structure for the given URL.
 * Default settings include HTTP version 1.1 and no write file descriptor.
 * The new handle is added to a global list of active handles.
 *
 * @param url The URL string for this connection. Must not be NULL.
 * @return A new Ecore_Con_Url handle, or NULL on failure.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Ecore_Con_Url *
ecore_con_url_new(const char *url)
{
   Ecore_Con_Url *url_con;

   EINA_SAFETY_ON_NULL_RETURN_VAL(url, NULL);

   url_con = calloc(1, sizeof(Ecore_Con_Url));
   EINA_SAFETY_ON_NULL_RETURN_VAL(url_con, NULL);

   url_con->url = eina_stringshare_add(url);
   url_con->http_version = EFL_NET_HTTP_VERSION_V1_1;
   url_con->write_fd = -1;

   EINA_MAGIC_SET(url_con, ECORE_MAGIC_CON_URL);
   _url_con_url_list = eina_list_append(_url_con_url_list, url_con);

   return url_con;
}

/**
 * @brief Creates a new Ecore_Con_Url handle with a custom HTTP request method.
 *
 * This is similar to ecore_con_url_new(), but allows specifying a custom
 * HTTP request method (e.g., "DELETE", "OPTIONS").
 *
 * @param url The URL string for this connection. Must not be NULL.
 * @param custom_request The custom HTTP request method string. Must not be NULL.
 * @return A new Ecore_Con_Url handle, or NULL on failure.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Ecore_Con_Url *
ecore_con_url_custom_new(const char *url,
                         const char *custom_request)
{
   Ecore_Con_Url *url_con;

   EINA_SAFETY_ON_NULL_RETURN_VAL(url, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(custom_request, NULL);

   url_con = ecore_con_url_new(url);
   EINA_SAFETY_ON_NULL_RETURN_VAL(url_con, NULL);

   url_con->custom_request = eina_stringshare_add(custom_request);

   return url_con;
}

/**
 * @brief Frees an Ecore_Con_Url handle and its associated resources.
 *
 * Removes the handle from the global list and calls the internal free function.
 * If there are pending events for this handle, the actual freeing might be
 * deferred until all events are processed.
 *
 * @param url_con The Ecore_Con_Url handle to free.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_free(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   /* remove from list as early as possible, we don't want to call
    * ecore_con_url_free() again on pending handles in ecore_con_url_shutdown()
    */
   _url_con_url_list = eina_list_remove(_url_con_url_list, url_con);
   _ecore_con_url_free_internal(url_con);
}

/**
 * @brief Gets the user-specific data associated with an Ecore_Con_Url handle.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return The user data pointer, or NULL if not set or on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void *
ecore_con_url_data_get(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, NULL);
   return url_con->data;
}

/**
 * @brief Sets user-specific data for an Ecore_Con_Url handle.
 *
 * This allows associating arbitrary data with the connection handle,
 * which can be retrieved later using ecore_con_url_data_get().
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param data The user data pointer to set.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_data_set(Ecore_Con_Url *url_con,
                       void *data)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   url_con->data = data;
}

/**
 * @brief Sets or updates the URL for an Ecore_Con_Url handle.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param url The new URL string. If NULL, the URL is set to an empty string.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_url_set(Ecore_Con_Url *url_con,
                      const char *url)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   eina_stringshare_replace(&url_con->url, url ? url : "");
   return EINA_TRUE;
}

/**
 * @brief Gets the URL string from an Ecore_Con_Url handle.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return The URL string, or NULL if not set or on error. The returned
 *         string is an Eina_Stringshare and should not be freed by the caller.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API const char *
ecore_con_url_url_get(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, NULL);
   return url_con->url;
}

/* LEGACY: HTTP requests */
/**
 * @brief Initiates an HTTP GET request.
 *
 * Prepares the Ecore_Con_Url handle for a "GET" request and starts the dial operation.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return EINA_TRUE if the request was successfully initiated, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_get(Ecore_Con_Url *url_con)
{
   Eina_Error err;

   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);

   if (!_ecore_con_url_request_prepare(url_con, "GET"))
     return EINA_FALSE;

   err = efl_net_dialer_dial(url_con->dialer, url_con->url);
   if (err)
     {
        WRN("failed to HTTP GET '%s': %s", url_con->url, eina_error_msg_get(err));
        _ecore_con_url_dialer_close(url_con);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Initiates an HTTP HEAD request.
 *
 * Prepares the Ecore_Con_Url handle for a "HEAD" request and starts the dial operation.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return EINA_TRUE if the request was successfully initiated, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_head(Ecore_Con_Url *url_con)
{
   Eina_Error err;

   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);

   if (!_ecore_con_url_request_prepare(url_con, "HEAD"))
     return EINA_FALSE;

   err = efl_net_dialer_dial(url_con->dialer, url_con->url);
   if (err)
     {
        WRN("failed to HTTP HEAD '%s': %s", url_con->url, eina_error_msg_get(err));
        _ecore_con_url_dialer_close(url_con);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Initiates an HTTP POST request with the given data.
 *
 * Prepares the Ecore_Con_Url handle for a "POST" request. The provided data
 * is copied into an Efl_Io_Buffer, and an Efl_Io_Copier is set up to send
 * this data to the server after the connection is established.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param data Pointer to the data to be posted.
 * @param length The length of the data in bytes.
 * @param content_type The "Content-Type" header value for the POST data (e.g., "application/x-www-form-urlencoded").
 *                     If NULL, no Content-Type header is added by this function for the data.
 * @return EINA_TRUE if the request was successfully initiated, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_post(Ecore_Con_Url *url_con,
                   const void *data,
                   long length,
                   const char *content_type)
{
   Eo *buffer, *copier;
   Eina_Slice slice = { .mem = data, .len = length };
   Eina_Error err;

   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);

   if (!_ecore_con_url_request_prepare(url_con, "POST"))
     return EINA_FALSE;

   if (content_type)
     efl_net_dialer_http_request_header_add(url_con->dialer, "Content-Type", content_type);

   buffer = efl_add(EFL_IO_BUFFER_CLASS, efl_loop_get(url_con->dialer),
                    efl_name_set(efl_added, "post-buffer"),
                    efl_io_closer_close_on_invalidate_set(efl_added, EINA_TRUE),
                    efl_io_closer_close_on_exec_set(efl_added, EINA_TRUE));
   EINA_SAFETY_ON_NULL_GOTO(buffer, error_buffer);

   err = efl_io_writer_write(buffer, &slice, NULL);
   if (err)
     {
        WRN("could not populate buffer %p with %ld bytes: %s",
            buffer, length, eina_error_msg_get(err));
        goto error_copier;
     }

   copier = efl_add(EFL_IO_COPIER_CLASS, efl_loop_get(url_con->dialer),
                    efl_name_set(efl_added, "send-copier"),
                    efl_io_copier_source_set(efl_added, buffer),
                    efl_io_copier_destination_set(efl_added, url_con->dialer),
                    efl_io_closer_close_on_invalidate_set(efl_added, EINA_FALSE),
                    efl_event_callback_array_add(efl_added, _ecore_con_url_copier_cbs(), url_con));
   EINA_SAFETY_ON_NULL_GOTO(copier, error_copier);

   err = efl_net_dialer_dial(url_con->dialer, url_con->url);
   if (err)
     {
        WRN("failed to post to '%s': %s", url_con->url, eina_error_msg_get(err));
        goto error_dialer;
     }

   url_con->input = buffer;
   url_con->send_copier = copier;
   DBG("posting to '%s' using an Efl.Io.Copier=%p", url_con->url, copier);

   return EINA_TRUE;

 error_dialer:
   efl_del(copier);
 error_copier:
   efl_del(buffer);
 error_buffer:
   _ecore_con_url_dialer_close(url_con);
   return EINA_FALSE;
}

/* LEGACY: headers */
/**
 * @brief Adds a custom header to be sent with the HTTP request.
 *
 * The header is stored internally and will be applied when the request
 * is prepared by _ecore_con_url_request_prepare().
 * Multiple headers can be added by calling this function repeatedly.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param key The header key (e.g., "User-Agent"). Must not be NULL.
 * @param value The header value. Must not be NULL.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_additional_header_add(Ecore_Con_Url *url_con,
                                    const char *key,
                                    const char *value)
{
   Efl_Net_Http_Header *header;
   char *s;

   ECORE_CON_URL_CHECK_RETURN(url_con);
   EINA_SAFETY_ON_NULL_RETURN(key);
   EINA_SAFETY_ON_NULL_RETURN(value);

   header = malloc(sizeof(Efl_Net_Http_Header) +
                   strlen(key) + 1 +
                   strlen(value) + 1);
   EINA_SAFETY_ON_NULL_RETURN(header);

   header->key = s = (char *)header + sizeof(Efl_Net_Http_Header);
   memcpy(s, key, strlen(key) + 1);

   header->value = s = s + strlen(key) + 1;
   memcpy(s, value, strlen(value) + 1);

   url_con->request_headers = eina_list_append(url_con->request_headers,
                                               header);
}

/**
 * @brief Clears all previously added custom request headers.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_additional_headers_clear(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   _ecore_con_url_request_headers_free(url_con);
}

/**
 * @brief Sets a time condition for the HTTP request.
 *
 * This can be used to send "If-Modified-Since" or "If-Unmodified-Since" headers.
 * The condition and timestamp are stored and applied when the request is prepared.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param time_condition The type of time condition (ECORE_CON_URL_TIME_IFMODSINCE or ECORE_CON_URL_TIME_IFUNMODSINCE).
 *                       Use ECORE_CON_URL_TIME_NONE to disable.
 * @param timestamp The timestamp (seconds since epoch) to use for the condition.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_time(Ecore_Con_Url *url_con,
                   Ecore_Con_Url_Time time_condition,
                   double timestamp)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   url_con->time.condition = time_condition;
   url_con->time.stamp = timestamp;
}

/* LEGACY: cookies */
/**
 * @brief Initializes cookie handling for the Ecore_Con_Url handle.
 *
 * This function enables the cookie engine by setting an empty string
 * for the cookie file, which tells libcurl to just enable the engine
 * but not read from/write to any specific file yet.
 * The setting is persisted in `url_con->cookies.files` and applied
 * to the underlying CURL handle if a dialer already exists.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_init(Ecore_Con_Url *url_con)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);

   /* meaningful before and after dial, persist and apply */
   url_con->cookies.files = eina_list_append(url_con->cookies.files, eina_stringshare_add(""));

   if (!url_con->dialer) return;

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIEFILE, "");
}

/**
 * @brief Adds a file from which to read cookies.
 *
 * The specified file will be read by libcurl to load cookies.
 * This can be called multiple times to read cookies from several files.
 * The setting is persisted and applied to the underlying CURL handle
 * if a dialer already exists.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param file_name Path to the cookie file. Must not be NULL.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_file_add(Ecore_Con_Url *url_con,
                               const char * const file_name)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);
   EINA_SAFETY_ON_NULL_RETURN(file_name);

   /* meaningful before and after dial, persist and apply */
   url_con->cookies.files = eina_list_append(url_con->cookies.files, eina_stringshare_add(file_name));

   if (!url_con->dialer) return;

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIEFILE, file_name);
}

/**
 * @brief Clears all cookies from the current session's in-memory cookie store.
 *
 * If a connection is active, this command is sent to libcurl immediately.
 * Otherwise, the command "ALL" is queued to be executed when the
 * connection is prepared.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_clear(Ecore_Con_Url *url_con)
{
   static const char cookielist_cmd_all[] = "ALL";
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);

   /* only meaningful once, if not dialed, queue, otherwise execute */
   if (!url_con->dialer)
     {
        url_con->cookies.cmds = eina_list_append(url_con->cookies.cmds, cookielist_cmd_all);
        return;
     }

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIELIST, cookielist_cmd_all);
}

/**
 * @brief Clears all session cookies from the current session's in-memory cookie store.
 *
 * Session cookies are those that have no expiry date.
 * If a connection is active, this command is sent to libcurl immediately.
 * Otherwise, the command "SESS" is queued to be executed when the
 * connection is prepared.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_session_clear(Ecore_Con_Url *url_con)
{
   static const char cookielist_cmd_sess[] = "SESS";
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);

   /* only meaningful once, if not dialed, queue, otherwise execute */
   if (!url_con->dialer)
     {
        url_con->cookies.cmds = eina_list_append(url_con->cookies.cmds, cookielist_cmd_sess);
        return;
     }

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIELIST, cookielist_cmd_sess);
}

/**
 * @brief Sets whether to ignore cookies from previous sessions.
 *
 * If `ignore` is EINA_TRUE, libcurl will start a new cookie session,
 * ignoring all cookies that were loaded from files and marked as session cookies.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param ignore EINA_TRUE to ignore old session cookies, EINA_FALSE otherwise.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_ignore_old_session_set(Ecore_Con_Url *url_con,
                                             Eina_Bool ignore)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   url_con->cookies.ignore_old_session = ignore;
}

/**
 * @brief Sets the file to be used as a cookie jar.
 *
 * Libcurl will write all internally known cookies to this file when the
 * connection handle is closed (or when ecore_con_url_cookies_jar_write is called).
 * The setting is persisted and applied to the underlying CURL handle
 * if a dialer already exists.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param cookiejar_file Path to the cookie jar file. Must not be NULL.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_cookies_jar_file_set(Ecore_Con_Url *url_con,
                                   const char * const cookiejar_file)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cookiejar_file, EINA_FALSE);

   /* meaningful before and after dial, persist and apply */
   eina_stringshare_replace(&url_con->cookies.jar, cookiejar_file);

   if (!url_con->dialer) return EINA_TRUE;

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(curl_easy, EINA_FALSE);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIEJAR, url_con->cookies.jar);
   return EINA_TRUE;
}

/**
 * @brief Forces libcurl to write all known cookies to the cookie jar file.
 *
 * This function is only effective if a cookie jar file has been set using
 * ecore_con_url_cookies_jar_file_set() and a connection is active.
 * It sends the "FLUSH" command via CURLOPT_COOKIELIST.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_cookies_jar_write(Ecore_Con_Url *url_con)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);

   /* only meaningful after dialed */
   if (!url_con->dialer) return;

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_COOKIELIST, "FLUSH");
}

/* LEGACY: file upload/download */
/**
 * @brief Sets a file descriptor to which downloaded data will be written.
 *
 * If a valid file descriptor (>= 0) is set, data received from the URL
 * will be written directly to this fd instead of generating
 * ECORE_CON_EVENT_URL_DATA events. Set to -1 to disable writing to an fd
 * and revert to event-based data delivery.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param fd The file descriptor to write to, or -1 to disable.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_fd_set(Ecore_Con_Url *url_con, int fd)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);

   if (url_con->write_fd == fd) return;

   url_con->write_fd = fd;
   if (!url_con->dialer) return;
}

/**
 * @brief Initiates an FTP upload of a local file.
 *
 * Prepares the Ecore_Con_Url handle for a "PUT" request (used for FTP upload).
 * The target URL is constructed by appending the `upload_dir` (if provided)
 * and the basename of the `filename` to the base URL set in `url_con`.
 * An Efl_Io_File is used as the source for an Efl_Io_Copier to send the file data.
 *
 * @param url_con The Ecore_Con_Url handle. Its `url` field should be the base FTP path (e.g., "ftp://server.com/").
 * @param filename Path to the local file to upload. Must not be NULL or empty.
 * @param user Username for FTP authentication (can be NULL).
 * @param pass Password for FTP authentication (can be NULL).
 * @param upload_dir Optional subdirectory on the FTP server where the file should be placed (can be NULL).
 * @return EINA_TRUE if the upload was successfully initiated, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_ftp_upload(Ecore_Con_Url *url_con,
                         const char *filename,
                         const char *user,
                         const char *pass,
                         const char *upload_dir)
{
   char tmp[4096];
   char *bname;
   Eo *file, *copier;
   Eina_Error err;

   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(filename, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(filename[0] == '\0', EINA_FALSE);

   bname = strdup(filename);
   if (upload_dir)
     snprintf(tmp, sizeof(tmp), "%s/%s/%s", url_con->url, upload_dir, basename(bname));
   else
     snprintf(tmp, sizeof(tmp), "%s/%s", url_con->url, basename(bname));
   free(bname);

   if (!_ecore_con_url_request_prepare(url_con, "PUT"))
     return EINA_FALSE;

   efl_net_dialer_http_authentication_set(url_con->dialer, user, pass, EFL_NET_HTTP_AUTHENTICATION_METHOD_ANY, EINA_FALSE);

   file = efl_add(EFL_IO_FILE_CLASS, efl_loop_get(url_con->dialer),
                  efl_name_set(efl_added, "upload-file"),
                  efl_file_set(efl_added, filename),
                  efl_io_file_flags_set(efl_added, O_RDONLY),
                  efl_io_closer_close_on_invalidate_set(efl_added, EINA_TRUE),
                  efl_io_closer_close_on_exec_set(efl_added, EINA_TRUE));
   EINA_SAFETY_ON_NULL_GOTO(file, error_file);

   copier = efl_add(EFL_IO_COPIER_CLASS, efl_loop_get(url_con->dialer),
                    efl_name_set(efl_added, "send-copier"),
                    efl_io_copier_source_set(efl_added, file),
                    efl_io_copier_destination_set(efl_added, url_con->dialer),
                    efl_io_closer_close_on_invalidate_set(efl_added, EINA_FALSE),
                    efl_event_callback_array_add(efl_added, _ecore_con_url_copier_cbs(), url_con));
   EINA_SAFETY_ON_NULL_GOTO(copier, error_copier);

   err = efl_net_dialer_dial(url_con->dialer, tmp);
   if (err)
     {
        WRN("failed to upload file '%s' to '%s': %s", filename, tmp, eina_error_msg_get(err));
        goto error_dialer;
     }

   url_con->input = file;
   url_con->send_copier = copier;
   DBG("uploading file '%s' to '%s' using an Efl.Io.Copier=%p", filename, tmp, copier);

   return EINA_TRUE;

 error_dialer:
   efl_del(copier);
 error_copier:
   efl_del(file);
 error_file:
   _ecore_con_url_dialer_close(url_con);
   return EINA_FALSE;
}

/**
 * @brief Sets whether to use EPSV (Extended Passive Mode) for FTP transfers.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param use_epsv EINA_TRUE to use EPSV, EINA_FALSE to use PASV.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_ftp_use_epsv_set(Ecore_Con_Url *url_con,
                               Eina_Bool use_epsv)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   url_con->ftp_use_epsv = use_epsv;
}

/**
 * @brief Limits the maximum upload speed for the connection.
 *
 * This setting is applied directly to the underlying CURL handle if a
 * connection is active.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param max_speed Maximum send speed in bytes per second. 0 for unlimited.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_limit_upload_speed(Ecore_Con_Url *url_con, off_t max_speed)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);
   EINA_SAFETY_ON_NULL_RETURN(_c);

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_MAX_SEND_SPEED_LARGE, max_speed);
}

/**
 * @brief Limits the maximum download speed for the connection.
 *
 * This setting is applied directly to the underlying CURL handle if a
 * connection is active.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param max_speed Maximum receive speed in bytes per second. 0 for unlimited.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_limit_download_speed(Ecore_Con_Url *url_con, off_t max_speed)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);
   EINA_SAFETY_ON_NULL_RETURN(_c);

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   _c->curl_easy_setopt(curl_easy, CURLOPT_MAX_RECV_SPEED_LARGE, max_speed);
}

/* LEGACY: proxy */
/**
 * @brief Sets the password for proxy authentication.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param password The proxy password. Must not be NULL.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_proxy_password_set(Ecore_Con_Url *url_con, const char *password)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(password, EINA_FALSE);
   eina_stringshare_replace(&url_con->proxy.password, password);
   return EINA_TRUE;
}

/**
 * @brief Sets the username for proxy authentication.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param username The proxy username. Must not be NULL.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_proxy_username_set(Ecore_Con_Url *url_con, const char *username)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(username, EINA_FALSE);
   eina_stringshare_replace(&url_con->proxy.username, username);
   return EINA_TRUE;
}

/**
 * @brief Sets the proxy server URL.
 *
 * If `proxy_url` is NULL, proxy usage will be determined by environment
 * variables (e.g., http_proxy). Otherwise, the specified URL will be used.
 * The URL can be in the format "protocol://host:port" or just "host:port"
 * (in which case "http" is assumed as the protocol).
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param proxy_url The proxy server URL, or NULL to use environment settings.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_proxy_set(Ecore_Con_Url *url_con, const char *proxy_url)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   eina_stringshare_replace(&url_con->proxy.url, proxy_url);
   return EINA_TRUE;
}

/* LEGACY: response */
/**
 * @brief Gets the total number of bytes received so far for the current transfer.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return The number of received bytes, or EINA_FALSE (0) on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API int
ecore_con_url_received_bytes_get(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   return url_con->received_bytes;
}

/**
 * @brief Gets the HTTP status code of the response.
 *
 * This is typically valid after an ECORE_CON_EVENT_URL_COMPLETE event has been received.
 * If the connection is active, it queries the dialer; otherwise, it returns the stored status.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return The HTTP status code, or 0 if not yet available or on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API int
ecore_con_url_status_code_get(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, 0);
   if (!url_con->dialer) return url_con->status;
   return efl_net_dialer_http_response_status_get(url_con->dialer);
}

/**
 * @brief Gets the list of response headers received from the server.
 *
 * The list contains strings, where each string is a full header line
 * (e.g., "Content-Type: text/html\r\n"). The list and its contents
 * are owned by the Ecore_Con_Url handle and should not be modified or freed.
 * This is typically valid after an ECORE_CON_EVENT_URL_COMPLETE event or
 * after headers_done callback.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @return A const Eina_List of response header strings, or NULL on error or if no headers.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API const Eina_List *
ecore_con_url_response_headers_get(Ecore_Con_Url *url_con)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, NULL);
   return url_con->response_headers;
}

/* LEGACY: SSL */
/**
 * @brief Sets the path to the Certificate Authority (CA) bundle file or directory.
 *
 * This is used for verifying the peer's SSL certificate.
 * Setting a `ca_path` also implicitly enables peer verification
 * (`ssl_verify_peer` is set to EINA_TRUE).
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param ca_path Path to the CA file or directory. If NULL, peer verification might be disabled
 *                depending on `ecore_con_url_ssl_verify_peer_set`.
 * @return 0 on success, -1 on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API int
ecore_con_url_ssl_ca_set(Ecore_Con_Url *url_con,
                         const char *ca_path)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, -1);
   eina_stringshare_replace(&url_con->ca_path, ca_path);
   url_con->ssl_verify_peer = !!ca_path;
   return 0;
}

/**
 * @brief Enables or disables SSL peer certificate verification.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param verify EINA_TRUE to enable verification, EINA_FALSE to disable.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_ssl_verify_peer_set(Ecore_Con_Url *url_con,
                                  Eina_Bool verify)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);
   url_con->ssl_verify_peer = !!verify;
}

/* LEGACY: misc */
/**
 * @brief Sets HTTP authentication credentials.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param username The username for authentication. Must not be NULL.
 * @param password The password for authentication. Must not be NULL.
 * @param safe If EINA_TRUE, only "safe" authentication methods (like Digest) are considered
 *             (EFL_NET_HTTP_AUTHENTICATION_METHOD_ANY_SAFE).
 *             If EINA_FALSE, any method including Basic might be used
 *             (EFL_NET_HTTP_AUTHENTICATION_METHOD_ANY).
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_httpauth_set(Ecore_Con_Url *url_con,
                           const char *username,
                           const char *password,
                           Eina_Bool safe)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(username, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(password, EINA_FALSE);

   eina_stringshare_replace(&url_con->httpauth.username, username);
   eina_stringshare_replace(&url_con->httpauth.password, password);
   url_con->httpauth.method = safe ? EFL_NET_HTTP_AUTHENTICATION_METHOD_ANY_SAFE : EFL_NET_HTTP_AUTHENTICATION_METHOD_ANY;
   url_con->httpauth.restricted = safe;
   return EINA_TRUE;
}

/**
 * @brief Sets the HTTP protocol version to use for the connection.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param version The HTTP version (ECORE_CON_URL_HTTP_VERSION_1_0 or ECORE_CON_URL_HTTP_VERSION_1_1).
 * @return EINA_TRUE on success, EINA_FALSE if an unknown version is provided.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API Eina_Bool
ecore_con_url_http_version_set(Ecore_Con_Url *url_con, Ecore_Con_Url_Http_Version version)
{
   ECORE_CON_URL_CHECK_RETURN(url_con, EINA_FALSE);
   switch (version)
     {
      case ECORE_CON_URL_HTTP_VERSION_1_0:
         url_con->http_version = EFL_NET_HTTP_VERSION_V1_0;
         break;
      case ECORE_CON_URL_HTTP_VERSION_1_1:
         url_con->http_version = EFL_NET_HTTP_VERSION_V1_1;
         break;
      default:
         ERR("unknown HTTP version enum value %d", version);
         return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function for the connection timeout timer.
 *
 * This function is invoked when the Ecore_Timer set by `ecore_con_url_timeout_set`
 * expires. It logs a warning, determines an appropriate error status,
 * and triggers an ECORE_CON_EVENT_URL_COMPLETE event.
 *
 * @param data The Ecore_Con_Url handle associated with the timed-out connection.
 * @return EINA_FALSE to stop the timer (it's a one-shot timer).
 */
static Eina_Bool
_ecore_con_url_timeout_cb(void *data)
{
   Ecore_Con_Url *url_con = data;
   int status;

   url_con->timer = NULL;

   WRN("HTTP timeout url='%s'", efl_net_dialer_address_dial_get(url_con->dialer));

   status = efl_net_dialer_http_response_status_get(url_con->dialer);
   if ((status < 500) || (status > 599))
     {
        DBG("HTTP error %d reset to 1", status);
        status = 1; /* not a real HTTP error */
     }

   _ecore_con_event_url_complete_add(url_con, status);

   return EINA_FALSE;
}

/**
 * @brief Sets a timeout for the entire connection operation.
 *
 * If the timeout is reached before the connection completes, an
 * ECORE_CON_EVENT_URL_COMPLETE event will be generated with an error status.
 * A timeout value of 0.0 or less disables the timeout.
 *
 * Note: This function starts the timer immediately, which is a behavior
 * retained for compatibility with the legacy API.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param timeout The timeout duration in seconds.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_timeout_set(Ecore_Con_Url *url_con, double timeout)
{
   ECORE_CON_URL_CHECK_RETURN(url_con);

   if (url_con->timer)
     {
        ecore_timer_del(url_con->timer);
        url_con->timer = NULL;
     }

   if (timeout <= 0.0) return;

   // NOTE: it is weird to start the timeout right away here, but it
   // was done like that and we're keeping it for compatibility
   url_con->timer = ecore_timer_add(timeout, _ecore_con_url_timeout_cb, url_con);
}

/**
 * @brief Enables or disables verbose debugging output for the connection.
 *
 * When enabled, libcurl will produce detailed information about the transfer.
 * This setting is persisted and applied to the underlying CURL handle
 * if a dialer already exists. If verbose is enabled, it sets the CURL
 * debug function to NULL, meaning CURL's default verbose output (to stderr)
 * will be used, not Eina_Log.
 *
 * @param url_con The Ecore_Con_Url handle.
 * @param verbose EINA_TRUE to enable verbose output, EINA_FALSE to disable.
 * @ingroup Ecore_Con_Url_Group
 */
ECORE_CON_API void
ecore_con_url_verbose_set(Ecore_Con_Url *url_con,
                          Eina_Bool verbose)
{
   CURL *curl_easy;

   ECORE_CON_URL_CHECK_RETURN(url_con);

   /* meaningful before and after dial, persist and apply */
   url_con->verbose = !!verbose;

   if (!url_con->dialer) return;

   curl_easy = efl_net_dialer_http_curl_get(url_con->dialer);
   EINA_SAFETY_ON_NULL_RETURN(curl_easy);

   if (url_con->verbose)
     {
        _c->curl_easy_setopt(curl_easy, CURLOPT_DEBUGFUNCTION, NULL);
        _c->curl_easy_setopt(curl_easy, CURLOPT_DEBUGDATA, NULL);
        DBG("HTTP Dialer %p is set to legacy debug function (CURL's default, no eina_log)", url_con->dialer);
     }
   _c->curl_easy_setopt(curl_easy, CURLOPT_VERBOSE, (long)url_con->verbose);
}

/**
 * @}
 */

