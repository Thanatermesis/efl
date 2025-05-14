/**
 * @file
 * @brief This file defines an abstraction layer for libcurl.
 *
 * It includes necessary type definitions, enums, and function pointers
 * to interact with libcurl, either by linking against the system's
 * curl.h or by using a local replicated set of definitions.
 */
#ifndef ECORE_CON_URL_CURL_H
#define ECORE_CON_URL_CURL_H 1

#ifdef USE_CURL_H
/* During development you can set USE_CURL_H to use the system's
 * curl.h instead of the local replicated values, it will provide all
 * constants and type-checking.
 */
#include <curl/curl.h>
#else
#ifdef __WIN32__
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

// all the types, defines, enums etc. from curl that we actually USE.
// we have to add to this if we use more things from curl not already
// defined here. see curl headers to get them from

/**
 * @brief CURLMcode errors, returned from curl_multi_xxx functions.
 */
typedef enum
{
  CURLM_CALL_MULTI_PERFORM = -1, /**< Please call curl_multi_perform() or curl_multi_socket*() soon. */
  CURLM_OK,                      /**< Operation was successful. */
  CURLM_BAD_HANDLE,              /**< The passed-in handle is not a valid CURLM handle. */
  CURLM_BAD_EASY_HANDLE,       /**< An easy handle was not good/valid. */
  CURLM_OUT_OF_MEMORY,         /**< If you ever get this, you're in deep trouble. */
  CURLM_INTERNAL_ERROR,        /**< This is a libcurl bug. */
  CURLM_BAD_SOCKET,            /**< The passed in socket argument did not match. */
  CURLM_UNKNOWN_OPTION,        /**< curl_multi_setopt() with unsupported option. */
  CURLM_ADDED_ALREADY,         /**< An easy handle already added to a multi handle was attempted to get added - again. */
  CURLM_LAST                   /**< Last entry, never use. */
} CURLMcode;

#ifndef curl_socket_typedef
/* socket typedef */
#if defined(WIN32) && !defined(__LWIP_OPT_H__) && !defined(LWIP_HDR_OPT_H)
typedef SOCKET curl_socket_t; /**< Socket type for Windows. */
#define CURL_SOCKET_BAD INVALID_SOCKET /**< Invalid socket value for Windows. */
#else
typedef int curl_socket_t;    /**< Socket type for non-Windows systems. */
#define CURL_SOCKET_BAD -1    /**< Invalid socket value for non-Windows systems. */
#endif
#define curl_socket_typedef
#endif /* curl_socket_typedef */

/**
 * @brief Type of socket.
 */
typedef enum  {
  CURLSOCKTYPE_IPCXN,  /**< Socket created for a specific IP connection. */
  CURLSOCKTYPE_ACCEPT, /**< Socket created by accept() call. */
  CURLSOCKTYPE_LAST    /**< Last entry, never use. */
} curlsocktype;

/**
 * @brief Structure to hold socket address information.
 */
struct curl_sockaddr {
  int family;                 /**< Address family. */
  int socktype;               /**< Socket type. */
  int protocol;               /**< Protocol. */
  unsigned int addrlen;       /**< Length of the address.
                               *   addrlen was a socklen_t type before 7.18.0 but it
                               *   turned really ugly and painful on the systems that
                               *   lack this type.
                               */
  struct sockaddr addr;       /**< Actual socket address. */
};

#define CURL_POLL_NONE   0    /**< No event. */
#define CURL_POLL_IN     1    /**< Wait for input. */
#define CURL_POLL_OUT    2    /**< Wait for output. */
#define CURL_POLL_INOUT  3    /**< Wait for input or output. */
#define CURL_POLL_REMOVE 4    /**< Remove socket. */

#define CURL_SOCKET_TIMEOUT CURL_SOCKET_BAD /**< Socket timeout event. */

#define CURL_CSELECT_IN   0x01 /**< Wait for input. */
#define CURL_CSELECT_OUT  0x02 /**< Wait for output. */
#define CURL_CSELECT_ERR  0x04 /**< Wait for error. */

/**
 * @brief Information type for debug callback.
 */
typedef enum {
  CURLINFO_TEXT = 0,         /**< Informational text. */
  CURLINFO_HEADER_IN,        /**< Incoming header. */
  CURLINFO_HEADER_OUT,       /**< Outgoing header. */
  CURLINFO_DATA_IN,          /**< Incoming data. */
  CURLINFO_DATA_OUT,         /**< Outgoing data. */
  CURLINFO_SSL_DATA_IN,      /**< Incoming SSL data. */
  CURLINFO_SSL_DATA_OUT,     /**< Outgoing SSL data. */
  CURLINFO_END               /**< End marker. */
} curl_infotype;

/**
 * @brief CURLE_errors, returned from curl_easy_xxx functions.
 */
typedef enum {
  CURLE_OK = 0,                  /**< All fine. */
  CURLE_UNSUPPORTED_PROTOCOL,    /**< 1: The URL you passed to libcurl used a protocol that this libcurl does not support. */
  CURLE_FAILED_INIT,             /**< 2: Very early initialization code failed. */
  CURLE_URL_MALFORMAT,           /**< 3: The URL was not properly formatted. */
  CURLE_NOT_BUILT_IN,            /**< 4: A requested feature, protocol or option was not found built-in in this libcurl due to a build-time decision. */
  CURLE_COULDNT_RESOLVE_PROXY,   /**< 5: Couldn't resolve proxy. The given proxy host could not be resolved. */
  CURLE_COULDNT_RESOLVE_HOST,    /**< 6: Couldn't resolve host. The given remote host was not resolved. */
  CURLE_COULDNT_CONNECT,         /**< 7: Failed to connect() to host or proxy. */
  CURLE_FTP_WEIRD_SERVER_REPLY,  /**< 8: The server sent data libcurl couldn't parse. */
  CURLE_REMOTE_ACCESS_DENIED,    /**< 9: A service was denied by the server due to lack of access - when login fails this is not returned. */
  CURLE_FTP_ACCEPT_FAILED,       /**< 10: While waiting for the server to connect back when an active FTP session is used, an error code was sent by the server. */
  CURLE_FTP_WEIRD_PASS_REPLY,    /**< 11: After having sent the FTP password to the server, libcurl expects a proper reply. This error code indicates that an unexpected code was returned. */
  CURLE_FTP_ACCEPT_TIMEOUT,      /**< 12: During an active FTP session while waiting for the server to connect, the timeout expired. */
  CURLE_FTP_WEIRD_PASV_REPLY,    /**< 13: libcurl failed to get a sensible result back from the server as a response to either a PASV or a EPSV command. */
  CURLE_FTP_WEIRD_227_FORMAT,    /**< 14: FTP servers return a 227-line as a response to a PASV command. If libcurl fails to parse that line, this return code is passed back. */
  CURLE_FTP_CANT_GET_HOST,       /**< 15: An internal failure to lookup the host used for the new connection. */
  CURLE_HTTP2,                   /**< 16: A problem in the http2 framing layer. */
  CURLE_FTP_COULDNT_SET_TYPE,    /**< 17: Received an error when trying to set the transfer mode to binary or ASCII. */
  CURLE_PARTIAL_FILE,            /**< 18: A file transfer was shorter or larger than expected. */
  CURLE_FTP_COULDNT_RETR_FILE,   /**< 19: This was either a weird reply to a 'RETR' command or a zero byte transfer complete. */
  CURLE_OBSOLETE20,              /**< 20: NOT USED. */
  CURLE_QUOTE_ERROR,             /**< 21: When sending custom "QUOTE" commands to the remote server, one of the commands returned an error code that was 400 or higher. */
  CURLE_HTTP_RETURNED_ERROR,     /**< 22: This is returned if CURLOPT_FAILONERROR is set TRUE and the HTTP server returns an error code that is >= 400. */
  CURLE_WRITE_ERROR,             /**< 23: An error occurred when writing received data to a local file, or an error was returned to libcurl from a write callback. */
  CURLE_OBSOLETE24,              /**< 24: NOT USED. */
  CURLE_UPLOAD_FAILED,           /**< 25: Failed starting the upload. For FTP, the server typically denied the STOR command. */
  CURLE_READ_ERROR,              /**< 26: There was a problem reading a local file or an error returned by the read callback. */
  CURLE_OUT_OF_MEMORY,           /**< 27: A memory allocation request failed. */
  /* Note: CURLE_OUT_OF_MEMORY may sometimes indicate a conversion error
           instead of a memory allocation error if CURL_DOES_CONVERSIONS
           is defined
  */
  CURLE_OPERATION_TIMEDOUT,      /**< 28: Operation timeout. The specified time-out period was reached according to the conditions. */
  CURLE_OBSOLETE29,              /**< 29: NOT USED. */
  CURLE_FTP_PORT_FAILED,         /**< 30: The FTP PORT command returned error. */
  CURLE_FTP_COULDNT_USE_REST,    /**< 31: The FTP REST command returned error. This command is used for resumed FTP transfers. */
  CURLE_OBSOLETE32,              /**< 32: NOT USED. */
  CURLE_RANGE_ERROR,             /**< 33: The server does not support or accept range requests. */
  CURLE_HTTP_POST_ERROR,         /**< 34: This is an odd error that mainly occurs due to internal confusion. */
  CURLE_SSL_CONNECT_ERROR,       /**< 35: A problem occurred somewhere in the SSL/TLS handshake. */
  CURLE_BAD_DOWNLOAD_RESUME,     /**< 36: Couldn't resume download. This often means you tried to resume a download beyond the end of the file. */
  CURLE_FILE_COULDNT_READ_FILE,  /**< 37: A file given with FILE:// couldn't be opened. */
  CURLE_LDAP_CANNOT_BIND,        /**< 38: LDAP cannot bind. LDAP bind operation failed. */
  CURLE_LDAP_SEARCH_FAILED,      /**< 39: LDAP search failed. */
  CURLE_OBSOLETE40,              /**< 40: NOT USED. */
  CURLE_FUNCTION_NOT_FOUND,      /**< 41: Function not found. A required zlib function was not found. */
  CURLE_ABORTED_BY_CALLBACK,     /**< 42: Aborted by callback. A callback returned "abort" to libcurl. */
  CURLE_BAD_FUNCTION_ARGUMENT,   /**< 43: Internal error. A function was called with a bad parameter. */
  CURLE_OBSOLETE44,              /**< 44: NOT USED. */
  CURLE_INTERFACE_FAILED,        /**< 45: Interface error. A specified outgoing interface could not be used. */
  CURLE_OBSOLETE46,              /**< 46: NOT USED. */
  CURLE_TOO_MANY_REDIRECTS,      /**< 47: Too many redirects. When following redirects, libcurl hit the maximum amount. */
  CURLE_UNKNOWN_OPTION,          /**< 48: An option passed to libcurl is not recognized/known. */
  CURLE_TELNET_OPTION_SYNTAX,    /**< 49: A telnet option string was Illegally formatted. */
  CURLE_OBSOLETE50,              /**< 50: NOT USED. */
  CURLE_PEER_FAILED_VERIFICATION,/**< 51: The remote server's SSL certificate or SSH md5 fingerprint was deemed not OK. */
  CURLE_GOT_NOTHING,             /**< 52: Nothing was returned from the server, and under the circumstances, getting nothing is considered an error. */
  CURLE_SSL_ENGINE_NOTFOUND,     /**< 53: The specified crypto engine wasn't found. */
  CURLE_SSL_ENGINE_SETFAILED,    /**< 54: Failed setting the selected SSL crypto engine as default! */
  CURLE_SEND_ERROR,              /**< 55: Failed sending network data. */
  CURLE_RECV_ERROR,              /**< 56: Failure with receiving network data. */
  CURLE_OBSOLETE57,              /**< 57: NOT IN USE. */
  CURLE_SSL_CERTPROBLEM,         /**< 58: problem with the local client certificate. */
  CURLE_SSL_CIPHER,              /**< 59: Couldn't use specified cipher. */
  CURLE_SSL_CACERT,              /**< 60: Peer certificate cannot be authenticated with known CA certificates. */
  CURLE_BAD_CONTENT_ENCODING,    /**< 61: Unrecognized transfer encoding. */
  CURLE_LDAP_INVALID_URL,        /**< 62: Invalid LDAP URL. */
  CURLE_FILESIZE_EXCEEDED,       /**< 63: Maximum file size exceeded. */
  CURLE_USE_SSL_FAILED,          /**< 64: Requested FTP SSL level failed. */
  CURLE_SEND_FAIL_REWIND,        /**< 65: When doing a send operation curl had to rewind the data to retransmit, but the rewinding operation failed. */
  CURLE_SSL_ENGINE_INITFAILED,   /**< 66: Initiating the SSL Engine failed. */
  CURLE_LOGIN_DENIED,            /**< 67: The remote server denied curl to login. */
  CURLE_TFTP_NOTFOUND,           /**< 68: File not found on TFTP server. */
  CURLE_TFTP_PERM,               /**< 69: Permission problem on TFTP server. */
  CURLE_REMOTE_DISK_FULL,        /**< 70: Out of disk space on the server. */
  CURLE_TFTP_ILLEGAL,            /**< 71: Illegal TFTP operation. */
  CURLE_TFTP_UNKNOWNID,          /**< 72: Unknown TFTP transfer ID. */
  CURLE_REMOTE_FILE_EXISTS,      /**< 73: File already exists and will not be overwritten. */
  CURLE_TFTP_NOSUCHUSER,         /**< 74: This error should not occur transfer ID. */
  CURLE_CONV_FAILED,             /**< 75: Character conversion failed. */
  CURLE_CONV_REQD,               /**< 76: Caller must register conversion callbacks. */
  CURLE_SSL_CACERT_BADFILE,      /**< 77: Problem with reading the SSL CA cert (path? access rights?). */
  CURLE_REMOTE_FILE_NOT_FOUND,   /**< 78: The resource referenced in the URL does not exist. */
  CURLE_SSH,                     /**< 79: An unspecified error occurred during the SSH session. */
  CURLE_SSL_SHUTDOWN_FAILED,     /**< 80: Failed to shut down the SSL connection. */
  CURLE_AGAIN,                   /**< 81: Socket is not ready for send/recv wait till it's ready and try again. */
  CURLE_SSL_CRL_BADFILE,         /**< 82: Failed to load CRL file. */
  CURLE_SSL_ISSUER_ERROR,        /**< 83: Issuer check failed. */
  CURLE_FTP_PRET_FAILED,         /**< 84: FTP PRET command failed. */
  CURLE_RTSP_CSEQ_ERROR,         /**< 85: Mismatch of RTSP CSeq numbers. */
  CURLE_RTSP_SESSION_ERROR,      /**< 86: Mismatch of RTSP Session Identifiers. */
  CURLE_FTP_BAD_FILE_LIST,       /**< 87: Unable to parse FTP file list (during FTP wildcard downloading). */
  CURLE_CHUNK_FAILED,            /**< 88: Chunk callback reported error. */
  CURLE_NO_CONNECTION_AVAILABLE, /**< 89: No connection available, the session will be queued. */
  CURLE_SSL_PINNEDPUBKEYNOTMATCH,/**< 90: Specified pinned public key did not match. */
  CURLE_SSL_INVALIDCERTSTATUS,   /**< 91: Invalid certificate status. */
  CURLE_HTTP2_STREAM,            /**< 92: Stream error in HTTP/2 framing layer. */
  CURL_LAST                      /**< Last entry, never use! */
} CURLcode;

/** @name CURLOPTTYPE defines the type of arguments for curl_easy_setopt */
/** @{ */
#define CURLOPTTYPE_LONG          0     /**< Option takes a long argument. */
#define CURLOPTTYPE_OBJECTPOINT   10000 /**< Option takes a pointer argument. */
#define CURLOPTTYPE_STRINGPOINT   10000 /**< Option takes a pointer to a string argument. */
#define CURLOPTTYPE_FUNCTIONPOINT 20000 /**< Option takes a function pointer argument. */
#define CURLOPTTYPE_OFF_T         30000 /**< Option takes a curl_off_t argument. */
/** @} */
#define CINIT(na, t, nu) CURLOPT_ ## na = CURLOPTTYPE_ ## t + nu /**< Macro to initialize CURLoption values. */

/**
 * @brief Options for curl_easy_setopt.
 */
typedef enum
{
   CINIT(FILE, OBJECTPOINT, 1),             /**< Pass a FILE * to use as output. */
   CINIT(URL, OBJECTPOINT, 2),              /**< URL to work on. */
   CINIT(PROXY, OBJECTPOINT, 4),            /**< Proxy server string. */
   CINIT(USERPWD, OBJECTPOINT, 5),          /**< User name and password. */
   CINIT(INFILE, OBJECTPOINT, 9),           /**< Pass a FILE * to use as input. */
   CINIT(WRITEFUNCTION, FUNCTIONPOINT, 11), /**< Callback for writing data. */
   CINIT(READFUNCTION, FUNCTIONPOINT, 12),  /**< Callback for reading data. */
   CINIT(POSTFIELDS, OBJECTPOINT, 15),    /**< Data to POST. */
   CINIT(USERAGENT, STRINGPOINT, 18),       /**< User-Agent string. */
   CINIT(HTTPHEADER, OBJECTPOINT, 23),      /**< Custom HTTP headers. */
   CINIT(WRITEHEADER, OBJECTPOINT, 29),     /**< Header output FILE *. */
   CINIT(COOKIEFILE, OBJECTPOINT, 31),      /**< File to read cookies from. */
   CINIT(TIMECONDITION, LONG, 33),          /**< Time condition. */
   CINIT(TIMEVALUE, LONG, 34),              /**< Time value for TIMECONDITION. */
   CINIT(CUSTOMREQUEST, OBJECTPOINT, 36),   /**< Custom request/method. */
   CINIT(VERBOSE, LONG, 41),                /**< Verbose mode. */
   CINIT(NOPROGRESS, LONG, 43),             /**< Disable progress meter. */
   CINIT(NOBODY, LONG, 44),                 /**< Do a HEAD request. */
   CINIT(UPLOAD, LONG, 46),                 /**< Upload data. */
   CINIT(POST, LONG, 47),                   /**< Do a POST request. */
   CINIT(PUT, LONG, 54),                    /**< Do a PUT request. */
   CINIT(FOLLOWLOCATION, LONG, 52),         /**< Follow HTTP redirects. */
   CINIT(PROGRESSFUNCTION, FUNCTIONPOINT, 56),/**< Callback for progress meter. */
   CINIT(PROGRESSDATA, OBJECTPOINT, 57),    /**< User data for progress callback. */
   CINIT(POSTFIELDSIZE, LONG, 60),          /**< Size of POST data. */
   CINIT(SSL_VERIFYPEER, LONG, 64),         /**< Verify the peer's SSL certificate. */
   CINIT(CAINFO, OBJECTPOINT, 65),          /**< CA certificate bundle. */
   CINIT(CONNECTTIMEOUT, LONG, 78),         /**< Connection timeout in seconds. */
   CINIT(CONNECTTIMEOUT_MS, LONG, 156),     /**< Connection timeout in milliseconds. */
   CINIT(HEADERFUNCTION, FUNCTIONPOINT, 79),/**< Callback for writing received headers. */
   CINIT(HTTPGET, LONG, 80),                /**< Force GET request. */
   CINIT(SSL_VERIFYHOST, LONG, 81),         /**< Verify the host name in the SSL certificate. */
   CINIT(COOKIEJAR, OBJECTPOINT, 82),       /**< File to write cookies to. */
   CINIT(HTTP_VERSION, LONG, 84),           /**< HTTP version to use. */
   CINIT(FTP_USE_EPSV, LONG, 85),           /**< Use EPSV for FTP. */
   CINIT(DEBUGFUNCTION, FUNCTIONPOINT, 94), /**< Callback for debug information. */
   CINIT(DEBUGDATA, OBJECTPOINT, 95),       /**< User data for debug callback. */
   CINIT(COOKIESESSION, LONG, 96),          /**< Start a new cookie session. */
   CINIT(CAPATH, STRINGPOINT, 97),          /**< Path to CA certificate directory. */
   CINIT(BUFFERSIZE, LONG, 98),             /**< Set receive buffer size. */
   CINIT(NOSIGNAL, LONG, 99),               /**< Skip all signal handling. */
   CINIT(PROXYTYPE, LONG, 101),             /**< Type of proxy. */
   CINIT(ACCEPT_ENCODING, OBJECTPOINT, 102),/**< Accept-Encoding string. */
   CINIT(PRIVATE, OBJECTPOINT, 103),        /**< Private pointer for the application. */
   CINIT(HTTPAUTH, LONG, 107),              /**< HTTP authentication methods. */
   CINIT(INFILESIZE_LARGE, OFF_T, 115),     /**< Size of file to send (large version). */
   CINIT(POSTFIELDSIZE_LARGE, OFF_T, 120),  /**< Size of POST data (large version). */
   CINIT(COOKIELIST, OBJECTPOINT, 135),     /**< List of cookie strings. */
   CINIT(MAX_SEND_SPEED_LARGE, OFF_T, 145), /**< Max send speed (large version). */
   CINIT(MAX_RECV_SPEED_LARGE, OFF_T, 146), /**< Max receive speed (large version). */
   CINIT(OPENSOCKETFUNCTION, FUNCTIONPOINT, 163), /**< Callback to open a socket. */
   CINIT(OPENSOCKETDATA, OBJECTPOINT, 164), /**< User data for open socket callback. */
   CINIT(CRLFILE, STRINGPOINT, 169),        /**< Path to CRL file. */
   CINIT(USERNAME, OBJECTPOINT, 173),       /**< User name for authentication. */
   CINIT(PASSWORD, OBJECTPOINT, 174),       /**< Password for authentication. */
   CINIT(CLOSESOCKETFUNCTION, FUNCTIONPOINT, 208), /**< Callback to close a socket. */
   CINIT(CLOSESOCKETDATA, OBJECTPOINT, 209), /**< User data for close socket callback. */
   CINIT(XFERINFOFUNCTION, FUNCTIONPOINT, 219), /**< Callback for transfer information (replaces PROGRESSFUNCTION). */
#define CURLOPT_XFERINFODATA CURLOPT_PROGRESSDATA /**< User data for XFERINFOFUNCTION. */
} CURLoption;

/** @name CURLINFO defines for curl_easy_getinfo */
/** @{ */
#define CURLINFO_STRING   0x100000 /**< Argument is a char **. */
#define CURLINFO_LONG     0x200000 /**< Argument is a long *. */
#define CURLINFO_DOUBLE   0x300000 /**< Argument is a double *. */
#define CURLINFO_SLIST    0x400000 /**< Argument is a struct curl_slist **. */
#define CURLINFO_MASK     0x0fffff /**< Mask for the info ID. */
#define CURLINFO_TYPEMASK 0xf00000 /**< Mask for the info type. */
/** @} */

/**
 * @brief Information to retrieve with curl_easy_getinfo.
 */
typedef enum
{
   CURLINFO_EFFECTIVE_URL    = CURLINFO_STRING + 1,    /**< Last effective URL. */
   CURLINFO_RESPONSE_CODE    = CURLINFO_LONG + 2,      /**< Last response code. */
   CURLINFO_CONTENT_LENGTH_DOWNLOAD = CURLINFO_DOUBLE + 15, /**< Content-Length of download. */
   CURLINFO_CONTENT_TYPE     = CURLINFO_STRING + 18,   /**< Content-Type. */
   CURLINFO_PRIVATE          = CURLINFO_STRING + 21,   /**< Private data pointer. */
   CURLINFO_HTTP_VERSION     = CURLINFO_LONG   + 46,   /**< The http version used in the connection. */
   CURLINFO_OS_ERRNO         = CURLINFO_LONG   + 25,   /**< Errno from connect failure. */
   CURLINFO_LOCAL_IP         = CURLINFO_STRING + 41,   /**< Local IP address of connection. */
   CURLINFO_LOCAL_PORT       = CURLINFO_LONG   + 42,   /**< Local port of connection. */
} CURLINFO;

/**
 * @brief CURL version constants.
 */
typedef enum
{
   CURLVERSION_FOURTH = 3 /**< Fourth CURL version. */
} CURLversion;

/**
 * @brief Message structure for curl_multi_info_read.
 */
typedef enum
{
   CURLMSG_DONE = 1 /**< The transfer is done. */
} CURLMSG;
#undef CINIT
#define CINIT(name, type, num) CURLMOPT_ ## name = CURLOPTTYPE_ ## type + num /**< Macro to initialize CURLMoption values. */

/**
 * @brief Options for curl_multi_setopt.
 */
typedef enum
{
   CINIT(SOCKETFUNCTION, FUNCTIONPOINT, 1), /**< Callback for socket activity. */
   CINIT(SOCKETDATA, OBJECTPOINT, 2),       /**< User data for socket callback. */
   CINIT(PIPELINING, LONG, 3),              /**< Enable pipelining. */
   CINIT(TIMERFUNCTION, FUNCTIONPOINT, 4),  /**< Callback for timer. */
   CINIT(TIMERDATA, OBJECTPOINT, 5)         /**< User data for timer callback. */
} CURLMoption;

/**
 * @brief Time condition types for CURLOPT_TIMECONDITION.
 */
typedef enum
{
   CURL_TIMECOND_NONE = 0,           /**< Do not use a time condition. */
   CURL_TIMECOND_IFMODSINCE = 1,     /**< If-Modified-Since condition. */
   CURL_TIMECOND_IFUNMODSINCE = 2    /**< If-Unmodified-Since condition. */
} curl_TimeCond;

/**
 * @brief HTTP versions for CURLOPT_HTTP_VERSION.
 */
enum
{
   CURL_HTTP_VERSION_NONE = 0, /**< Default, let libcurl decide. */
   CURL_HTTP_VERSION_1_0 = 1,  /**< Enforce HTTP 1.0. */
   CURL_HTTP_VERSION_1_1 = 2,  /**< Enforce HTTP 1.1. */
   CURL_HTTP_VERSION_2_0 = 3   /**< Enforce HTTP 2.0. */
};

/**
 * @brief Proxy types for CURLOPT_PROXYTYPE.
 */
typedef enum
{
   CURLPROXY_HTTP = 0,             /**< HTTP proxy. */
   CURLPROXY_SOCKS4 = 4,           /**< SOCKS4 proxy. */
   CURLPROXY_SOCKS5 = 5,           /**< SOCKS5 proxy. */
   CURLPROXY_SOCKS4A = 6,          /**< SOCKS4a proxy. */
   CURLPROXY_SOCKS5_HOSTNAME = 7   /**< SOCKS5 proxy with hostname resolving. */
} curl_proxytype;

/** @name Flags for curl_global_init */
/** @{ */
#define CURL_GLOBAL_SSL     (1 << 0) /**< Initialize SSL. */
#define CURL_GLOBAL_WIN32   (1 << 1) /**< Initialize Win32 sockets. */
#define CURL_GLOBAL_ALL     (CURL_GLOBAL_SSL | CURL_GLOBAL_WIN32) /**< Initialize SSL and Win32 sockets. */
/** @} */

/** @name Aliases for common CURLOPT options */
/** @{ */
#define CURLOPT_ENCODING    CURLOPT_ACCEPT_ENCODING /**< Alias for CURLOPT_ACCEPT_ENCODING. */
#define CURLOPT_WRITEDATA   CURLOPT_FILE            /**< Alias for CURLOPT_FILE (for writing). */
#define CURLOPT_READDATA    CURLOPT_INFILE          /**< Alias for CURLOPT_INFILE (for reading). */
#define CURLOPT_HEADERDATA  CURLOPT_WRITEHEADER     /**< Alias for CURLOPT_WRITEHEADER. */
/** @} */

#define CURLVERSION_NOW     CURLVERSION_FOURTH /**< Current CURL version for curl_version_info(). */

/** @name Authentication methods for CURLOPT_HTTPAUTH */
/** @{ */
#define CURLAUTH_NONE       ((unsigned long)0)       /**< No authentication. */
#define CURLAUTH_BASIC      (((unsigned long)1) << 0)/**< HTTP Basic authentication. */
#define CURLAUTH_DIGEST       (((unsigned long)1)<<1)/**< HTTP Digest authentication. */
#define CURLAUTH_NEGOTIATE    (((unsigned long)1)<<2)/**< HTTP Negotiate (SPNEGO) authentication. */
#define CURLAUTH_DIGEST_IE  (((unsigned long)1) << 4)/**< HTTP Digest authentication with IE flavor. */
#define CURLAUTH_ANY        (~CURLAUTH_DIGEST_IE)    /**< Try any authentication method (except Digest IE). */
#define CURLAUTH_NTLM         (((unsigned long)1)<<3)/**< NTLM authentication. */
#define CURLAUTH_NTLM_WB      (((unsigned long)1)<<5)/**< NTLM authentication with winbind helper. */
#define CURLAUTH_ONLY         (((unsigned long)1)<<31)/**< Use only the specified authentication method. */
// #define CURLAUTH_ANY          (~CURLAUTH_DIGEST_IE) /* Redefined */
#define CURLAUTH_ANYSAFE      (~(CURLAUTH_BASIC|CURLAUTH_DIGEST_IE)) /**< Try any safe authentication method. */
/** @} */

/** @name Return codes for read callback */
/** @{ */
#define CURL_READFUNC_ABORT 0x10000000 /**< Abort the transfer. */
#define CURL_READFUNC_PAUSE 0x10000001 /**< Pause the transfer from the read callback. */
/** @} */

#define CURL_WRITEFUNC_PAUSE 0x10000001 /**< Pause the transfer from the write callback. */

/** @name Flags for curl_easy_pause */
/** @{ */
#define CURLPAUSE_RECV      (1<<0) /**< Pause receiving data. */
#define CURLPAUSE_RECV_CONT (0)    /**< Continue receiving data. */

#define CURLPAUSE_SEND      (1<<2) /**< Pause sending data. */
#define CURLPAUSE_SEND_CONT (0)    /**< Continue sending data. */
/** @} */

typedef void CURLM; /**< Opaque handle for a CURL multi interface. */
typedef void CURL;  /**< Opaque handle for a CURL easy interface. */

/**
 * @brief A singly linked list of strings.
 */
struct curl_slist
{
   char              *data; /**< Pointer to the string data. */
   struct curl_slist *next; /**< Pointer to the next list item. */
};

/**
 * @brief Structure holding version information for libcurl and its dependencies.
 */
typedef struct
{
   CURLversion        age;             /**< Age of this struct. */
   const char        *version;         /**< libcurl version string. */
   unsigned int       version_num;     /**< libcurl version number. */
   const char        *host;            /**< Information about the host libcurl was built on. */
   int                features;        /**< Bitmask of features. */
   const char        *ssl_version;     /**< SSL library version string. */
   long               ssl_version_num; /**< SSL library version number. */
   const char        *libz_version;    /**< zlib version string. */
   const char *const *protocols;       /**< Array of supported protocol strings. */
   const char        *ares;            /**< c-ares version string, if used. */
   int                ares_num;        /**< c-ares version number, if used. */
   const char        *libidn;          /**< libidn version string, if used. */
   int                iconv_ver_num;   /**< iconv version number, if used. */
   const char        *libssh_version;  /**< libssh version string, if used. */
} curl_version_info_data;

/**
 * @brief Message structure used by curl_multi_info_read().
 */
typedef struct
{
   CURLMSG msg;          /**< The message type. Currently only CURLMSG_DONE. */
   CURL   *easy_handle;  /**< The easy handle that completed. */
   union
   {
      void    *whatever; /**< Unused. */
      CURLcode result;   /**< The result of the transfer for the easy_handle. */
   } data;
} CURLMsg;

#endif /* USE_CURL_H */

/**
 * @brief Structure to hold loaded libcurl function pointers and state.
 *
 * This structure is used to dynamically load and manage libcurl functions
 * at runtime. It also holds the global CURLM handle.
 */
typedef struct _Ecore_Con_Curl Ecore_Con_Curl;

struct _Ecore_Con_Curl
{
   Eina_Module            *mod;    /**< Eina_Module handle for the loaded libcurl. */

   CURLM                  *_curlm; /**< Global CURL multi handle. */

   /* Function pointers for libcurl API */
   CURLcode                (*curl_global_init)(long flags); /**< See curl_global_init(). */
   void                    (*curl_global_cleanup)(void);    /**< See curl_global_cleanup(). */
   CURLM                  *(*curl_multi_init)(void);      /**< See curl_multi_init(). */
   CURLMcode               (*curl_multi_timeout)(CURLM *multi_handle, long *milliseconds); /**< See curl_multi_timeout(). */
   CURLMcode               (*curl_multi_cleanup)(CURLM *multi_handle); /**< See curl_multi_cleanup(). */
   CURLMcode               (*curl_multi_remove_handle)(CURLM *multi_handle, CURL *curl_handle); /**< See curl_multi_remove_handle(). */
   const char             *(*curl_multi_strerror)(CURLMcode); /**< See curl_multi_strerror(). */
   CURLMsg                *(*curl_multi_info_read)(CURLM * multi_handle, int *msgs_in_queue); /**< See curl_multi_info_read(). */
   CURLMcode               (*curl_multi_fdset)(CURLM *multi_handle, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd); /**< See curl_multi_fdset(). */
   CURLMcode               (*curl_multi_perform)(CURLM *multi_handle, int *running_handles); /**< See curl_multi_perform(). */
   CURLMcode               (*curl_multi_add_handle)(CURLM *multi_handle, CURL *curl_handle); /**< See curl_multi_add_handle(). */
   CURLMcode               (*curl_multi_setopt)(CURLM *multi_handle, CURLMoption option, ...); /**< See curl_multi_setopt(). */
   CURLMcode               (*curl_multi_socket_action)(CURLM *multi_handle, curl_socket_t fd, int ev_bitmask, int *running_handles); /**< See curl_multi_socket_action(). */
   CURLMcode               (*curl_multi_assign)(CURLM *multi_handle, curl_socket_t sockfd, void *sockp); /**< See curl_multi_assign(). */
   CURL                   *(*curl_easy_init)(void); /**< See curl_easy_init(). */
   CURLcode                (*curl_easy_setopt)(CURL *curl, CURLoption option, ...); /**< See curl_easy_setopt(). */
   const char             *(*curl_easy_strerror)(CURLcode); /**< See curl_easy_strerror(). */
   void                    (*curl_easy_cleanup)(CURL *curl); /**< See curl_easy_cleanup(). */
   CURLcode                (*curl_easy_getinfo)(CURL *curl, CURLINFO info, ...); /**< See curl_easy_getinfo(). */
   CURLcode                (*curl_easy_pause)(CURL *curl, int bitmask); /**< See curl_easy_pause(). */
   void                    (*curl_slist_free_all)(struct curl_slist *); /**< See curl_slist_free_all(). */
   struct curl_slist      *(*curl_slist_append)(struct curl_slist *list, const char *string); /**< See curl_slist_append(). */
   time_t                  (*curl_getdate)(const char *p, const time_t *unused); /**< See curl_getdate(). */
   curl_version_info_data *(*curl_version_info)(CURLversion); /**< See curl_version_info(). */

   int                     ref; /**< Reference count for this structure. */
};

#define CURL_MIN_TIMEOUT 100 /**< Minimum timeout value in milliseconds for curl_multi_timeout. */

extern Ecore_Con_Curl *_c; /**< Global pointer to the Ecore_Con_Curl structure. */
extern Eina_Bool _c_fail;  /**< Flag indicating if libcurl initialization failed. */
extern double _c_timeout;  /**< Timeout value derived from curl_multi_timeout. */

/**
 * @brief Initializes the libcurl wrapper.
 *
 * Loads libcurl dynamically and initializes its global state.
 * Increments a reference counter.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool _c_init(void);

/**
 * @brief Shuts down the libcurl wrapper.
 *
 * Decrements the reference counter. If it reaches zero,
 * cleans up libcurl global state and unloads the library.
 */
void _c_shutdown(void);

/**
 * @brief Converts a CURLcode to an Eina_Error.
 * @param code The CURLcode to convert.
 * @return The corresponding Eina_Error, or EINVAL if not mapped.
 */
Eina_Error _curlcode_to_eina_error(const CURLcode code);

/**
 * @brief Converts a CURLMcode to an Eina_Error.
 * @param code The CURLMcode to convert.
 * @return The corresponding Eina_Error, or EINVAL if not mapped.
 */
Eina_Error _curlmcode_to_eina_error(const CURLMcode code);


/* only for legacy support to implement behavior that we're not exposing anymore */
/**
 * @brief Retrieves the underlying CURL easy handle for a given Eo object.
 * @warning This function is for legacy support and should not be used in new code.
 * @param o The Eo object representing an HTTP dialer.
 * @return The CURL easy handle, or NULL if not applicable or on error.
 */
CURL *efl_net_dialer_http_curl_get(const Eo *o);

#endif
