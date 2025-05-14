#define EFL_NET_SSL_CONTEXT_PROTECTED 1
#define EFL_IO_READER_PROTECTED 1
#define EFL_IO_WRITER_PROTECTED 1
#define EFL_IO_CLOSER_PROTECTED 1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"

#include "Emile.h"

/**
 * This function is used by efl_net_socket_ssl to retrieve a new
 * connection based on the implementation-depentent context.
 *
 * @internal
 */
void *efl_net_ssl_context_connection_new(Efl_Net_Ssl_Context *context);

/**
 * @brief Opaque handle for the platform-specific SSL context implementation.
 * @internal
 *
 * This structure is defined in the platform-specific C files (e.g.,
 * efl_net_ssl_ctx-openssl.c) and holds the actual SSL context data
 * (like SSL_CTX* for OpenSSL).
 */
typedef struct _Efl_Net_Ssl_Ctx Efl_Net_Ssl_Ctx;

/**
 * @brief Configuration structure passed to the platform-specific SSL context setup.
 * @internal
 */
typedef struct _Efl_Net_Ssl_Ctx_Config {
   Efl_Net_Ssl_Cipher cipher; /**< The SSL/TLS cipher suite to use. */
   Eina_Bool is_dialer; /**< EINA_TRUE if this context is for a client (dialer), EINA_FALSE for a server (listener). */
   Eina_Bool load_defaults; /**< EINA_TRUE to load default CA certificates from system paths. */
   Eina_List **certificates; /**< Pointer to a list of paths to PEM-encoded certificate files. The list itself may be updated by the setup function. */
   Eina_List **private_keys; /**< Pointer to a list of paths to PEM-encoded private key files. The list itself may be updated. */
   Eina_List **certificate_revocation_lists; /**< Pointer to a list of paths to PEM-encoded CRL files. The list itself may be updated. */
   Eina_List **certificate_authorities; /**< Pointer to a list of paths to PEM-encoded CA certificate files. The list itself may be updated. */
} Efl_Net_Ssl_Ctx_Config;

/**
 * Returns the platform dependent context to efl_net_socket_ssl
 * wrapper.
 *
 * @internal
 */
static void *efl_net_ssl_ctx_connection_new(Efl_Net_Ssl_Ctx *ctx);

/**
 * Setups the SSL context
 *
 * Update the given lists, removing invalid entries. If all entries
 * failed in a list, return EINVAL.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_ctx_setup(Efl_Net_Ssl_Ctx *ctx, Efl_Net_Ssl_Ctx_Config cfg);

/**
 * Cleans up the SSL associated to this context.
 * @internal
 */
static void efl_net_ssl_ctx_teardown(Efl_Net_Ssl_Ctx *ctx);

/**
 * Configure how to verify peer.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_ctx_verify_mode_set(Efl_Net_Ssl_Ctx *ctx, Efl_Net_Ssl_Verify_Mode verify_mode);

/**
 * Configure whenever to check for hostname.
 *
 * @internal
 */
static Eina_Error efl_net_ssl_ctx_hostname_verify_set(Efl_Net_Ssl_Ctx *ctx, Eina_Bool hostname_verify);

/**
 * Configure the hostname to use.
 *
 * @note duplicate hostname if needed!
 *
 * @internal
 */
static Eina_Error efl_net_ssl_ctx_hostname_set(Efl_Net_Ssl_Ctx *ctx, const char *hostname);

#if HAVE_OPENSSL
# include "efl_net_ssl_ctx-openssl.c"
#else
# include "efl_net_ssl_ctx-none.c"
#endif

#define MY_CLASS EFL_NET_SSL_CONTEXT_CLASS

/**
 * @brief Private data for the Efl_Net_Ssl_Context Eo object.
 * @internal
 */
typedef struct _Efl_Net_Ssl_Context_Data
{
   Efl_Net_Ssl_Ctx ssl_ctx; /**< Platform-specific SSL context. */
   Eina_List *certificates; /**< List of eina_stringshare'd paths to PEM-encoded certificate files. */
   Eina_List *private_keys; /**< List of eina_stringshare'd paths to PEM-encoded private key files. */
   Eina_List *certificate_revocation_lists; /**< List of eina_stringshare'd paths to PEM-encoded CRL files. */
   Eina_List *certificate_authorities; /**< List of eina_stringshare'd paths to PEM-encoded CA certificate files. */
   const char *hostname; /**< eina_stringshare'd hostname to verify against the peer's certificate. */
   Efl_Net_Ssl_Cipher cipher; /**< SSL/TLS cipher suite configuration. */
   Eina_Bool is_dialer; /**< EINA_TRUE if the context is for a client (dialer). */
   Efl_Net_Ssl_Verify_Mode verify_mode; /**< Peer certificate verification mode. Initialized to 0xff (unset). */
   Eina_Bool load_defaults; /**< EINA_TRUE to load default CA paths. Initialized to 0xff (unset). */
   Eina_Bool hostname_verify; /**< EINA_TRUE to verify hostname. Initialized to 0xff (unset). */

   /* State flags, typically managed by the Efl.Net.Ssl.Socket layer, not directly here.
    * These might be legacy or for a different abstraction level.
    * For Efl.Net.Ssl.Context, the primary role is configuration.
    * Actual I/O state (handshake, read/write readiness, EOS) is usually
    * associated with an Efl_Net_Ssl_Socket instance using this context.
    */
   Eina_Bool did_handshake; /**< EINA_TRUE if SSL handshake completed. (Potentially managed by socket) */
   Eina_Bool can_read;      /**< EINA_TRUE if data can be read. (Potentially managed by socket) */
   Eina_Bool eos;           /**< EINA_TRUE if End-Of-Stream reached. (Potentially managed by socket) */
   Eina_Bool can_write;     /**< EINA_TRUE if data can be written. (Potentially managed by socket) */
} Efl_Net_Ssl_Context_Data;


void *
efl_net_ssl_context_connection_new(Efl_Net_Ssl_Context *context)
{
   Efl_Net_Ssl_Context_Data *pd = efl_data_scope_get(context, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);
   return efl_net_ssl_ctx_connection_new(&pd->ssl_ctx);
}

EOLIAN static void
_efl_net_ssl_context_setup(Eo *o, Efl_Net_Ssl_Context_Data *pd, Efl_Net_Ssl_Cipher cipher, Eina_Bool is_dialer)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   EINA_SAFETY_ON_TRUE_RETURN(cipher > EFL_NET_SSL_CIPHER_TLSV1_2);

   pd->cipher = cipher;
   pd->is_dialer = is_dialer;
}

/**
 * @brief Converts an Eina_Iterator of C strings to an Eina_List of eina_stringshare'd strings.
 * @internal
 *
 * This function iterates over the input iterator, stringshares each non-NULL string,
 * and appends it to a new Eina_List. The input iterator is freed upon completion.
 *
 * @param it The iterator providing C strings. Will be freed by this function.
 * @return A new Eina_List containing eina_stringshare'd versions of the input strings,
 *         or NULL if the input iterator was empty or on allocation failure.
 */
static Eina_List *
_efl_net_ssl_context_string_iter_to_list(Eina_Iterator *it)
{
   Eina_List *lst = NULL;
   const char *str;
   EINA_ITERATOR_FOREACH(it, str)
     {
        if (!str) continue;
        lst = eina_list_append(lst, eina_stringshare_add(str));
     }
   eina_iterator_free(it);
   return lst;
}

/**
 * @brief Frees an Eina_List of eina_stringshare'd strings.
 * @internal
 *
 * This function iterates over the list, deleting the stringshare for each string,
 * and then frees the list itself. The list pointer is set to NULL.
 *
 * @param p_lst Pointer to the Eina_List to be freed.
 */
static void
_efl_net_ssl_context_string_list_free(Eina_List **p_lst)
{
   const char *str;
   EINA_LIST_FREE(*p_lst, str)
     eina_stringshare_del(str);
}

static Eina_Iterator *
_efl_net_ssl_context_certificates_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return eina_list_iterator_new(pd->certificates);
}

static void
_efl_net_ssl_context_certificates_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Iterator *it)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   _efl_net_ssl_context_string_list_free(&pd->certificates);
   pd->certificates = _efl_net_ssl_context_string_iter_to_list(it);
}

static Eina_Iterator *
_efl_net_ssl_context_private_keys_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return eina_list_iterator_new(pd->private_keys);
}

static void
_efl_net_ssl_context_private_keys_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Iterator *it)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   _efl_net_ssl_context_string_list_free(&pd->private_keys);
   pd->private_keys = _efl_net_ssl_context_string_iter_to_list(it);
}

static Eina_Iterator *
_efl_net_ssl_context_certificate_revocation_lists_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return eina_list_iterator_new(pd->certificate_revocation_lists);
}

static void
_efl_net_ssl_context_certificate_revocation_lists_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Iterator *it)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   _efl_net_ssl_context_string_list_free(&pd->certificate_revocation_lists);
   pd->certificate_revocation_lists = _efl_net_ssl_context_string_iter_to_list(it);
}

static Eina_Iterator *
_efl_net_ssl_context_certificate_authorities_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return eina_list_iterator_new(pd->certificate_authorities);
}

static void
_efl_net_ssl_context_certificate_authorities_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Iterator *it)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   _efl_net_ssl_context_string_list_free(&pd->certificate_authorities);
   pd->certificate_authorities = _efl_net_ssl_context_string_iter_to_list(it);
}

static Eina_Bool
_efl_net_ssl_context_default_paths_load_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return pd->load_defaults;
}

static void
_efl_net_ssl_context_default_paths_load_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Bool load_defaults)
{
   EINA_SAFETY_ON_TRUE_RETURN(efl_finalized_get(o));
   pd->load_defaults = load_defaults;
}

static Efl_Net_Ssl_Verify_Mode
_efl_net_ssl_context_verify_mode_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return pd->verify_mode;
}

static void
_efl_net_ssl_context_verify_mode_set(Eo *o, Efl_Net_Ssl_Context_Data *pd, Efl_Net_Ssl_Verify_Mode verify_mode)
{
   pd->verify_mode = verify_mode;
   if (!efl_finalized_get(o)) return;

   efl_net_ssl_ctx_verify_mode_set(&pd->ssl_ctx, pd->verify_mode);
}

static Eina_Bool
_efl_net_ssl_context_hostname_verify_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return pd->hostname_verify;
}

static void
_efl_net_ssl_context_hostname_verify_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, Eina_Bool hostname_verify)
{
   pd->hostname_verify = hostname_verify;
   if (!efl_finalized_get(o)) return;

   efl_net_ssl_ctx_hostname_verify_set(&pd->ssl_ctx, pd->hostname_verify);
}

static const char *
_efl_net_ssl_context_hostname_get(const Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd)
{
   return pd->hostname;
}

static void
_efl_net_ssl_context_hostname_set(Eo *o EINA_UNUSED, Efl_Net_Ssl_Context_Data *pd, const char* hostname)
{
   eina_stringshare_replace(&pd->hostname, hostname);
   if (!efl_finalized_get(o)) return;

   efl_net_ssl_ctx_hostname_set(&pd->ssl_ctx, pd->hostname);
}

EOLIAN static Efl_Object *
_efl_net_ssl_context_efl_object_finalize(Eo *o, Efl_Net_Ssl_Context_Data *pd)
{
   Eina_Error err;
   Efl_Net_Ssl_Ctx_Config cfg;

   // Finalize the parent class first.
   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   if (!emile_cipher_init())
     {
        ERR("could not initialize cipher subsystem.");
        return NULL;
     }

   /*
    * Apply default settings if they haven't been explicitly set by the user.
    * The 0xff value is used as a sentinel to indicate "not set".
    * Dialer (client) contexts have stricter defaults (require verification, load default CAs).
    * Listener (server) contexts have lenient defaults (no verification by default).
    */
   if (pd->is_dialer)
     {
        // If verify_mode was not set, default to REQUIRED for dialers.
        if ((uint8_t)pd->verify_mode == 0xff)
          pd->verify_mode = EFL_NET_SSL_VERIFY_MODE_REQUIRED;
        // If hostname_verify was not set, default to TRUE for dialers.
        if (pd->hostname_verify == 0xff) // 0xff is EINA_TRUE_UNSET like
          pd->hostname_verify = EINA_TRUE;
        // If load_defaults was not set, default to TRUE for dialers.
        if (pd->load_defaults == 0xff) // 0xff is EINA_TRUE_UNSET like
          pd->load_defaults = EINA_TRUE;
     }
   else // Listener context
     {
        // If verify_mode was not set, default to NONE for listeners.
        if ((uint8_t)pd->verify_mode == 0xff)
          pd->verify_mode = EFL_NET_SSL_VERIFY_MODE_NONE;
        // If hostname_verify was not set, default to FALSE for listeners.
        if (pd->hostname_verify == 0xff)
          pd->hostname_verify = EINA_FALSE;
        // If load_defaults was not set, default to FALSE for listeners.
        if (pd->load_defaults == 0xff)
          pd->load_defaults = EINA_FALSE;
     }

   cfg.cipher = pd->cipher;
   cfg.is_dialer = pd->is_dialer;
   cfg.load_defaults = pd->load_defaults;
   cfg.certificates = &pd->certificates;
   cfg.private_keys = &pd->private_keys;
   cfg.certificate_revocation_lists = &pd->certificate_revocation_lists;
   cfg.certificate_authorities = &pd->certificate_authorities;

   err = efl_net_ssl_ctx_setup(&pd->ssl_ctx, cfg);
   if (err)
     {
        ERR("o=%p failed to setup context (is_dialer=%d)", o, cfg.is_dialer);
        return NULL;
     }
   DBG("o=%p setup context (is_dialer=%d) ssl_ctx=%p", o, cfg.is_dialer, &pd->ssl_ctx);

   efl_net_ssl_ctx_verify_mode_set(&pd->ssl_ctx, pd->verify_mode);
   efl_net_ssl_ctx_hostname_verify_set(&pd->ssl_ctx, pd->hostname_verify);
   efl_net_ssl_ctx_hostname_set(&pd->ssl_ctx, pd->hostname);

   return o;
}

EOLIAN static Eo *
_efl_net_ssl_context_efl_object_constructor(Eo *o, Efl_Net_Ssl_Context_Data *pd)
{
   // Initialize properties to default or "unset" states.
   pd->cipher = EFL_NET_SSL_CIPHER_AUTO; // Default cipher mode.
   pd->is_dialer = EINA_TRUE; // Default to a dialer (client) context.

   /*
    * Initialize boolean/enum properties that have specific defaults based on is_dialer
    * to 0xff. This sentinel value indicates that the user hasn't explicitly set them,
    * allowing _efl_net_ssl_context_efl_object_finalize to apply appropriate defaults.
    * 0xff is chosen as it's unlikely to be a valid value for these properties
    * and can represent an "unset" state for Eina_Bool-like fields if EINA_TRUE_UNSET (2) is not used.
    */
   pd->load_defaults = 0xff;   // Mark as unset, to be defaulted in finalize.
   pd->hostname_verify = 0xff; // Mark as unset, to be defaulted in finalize.
   pd->verify_mode = 0xff;     // Mark as unset, to be defaulted in finalize.

   return efl_constructor(efl_super(o, MY_CLASS));
}

EOLIAN static void
_efl_net_ssl_context_efl_object_destructor(Eo *o, Efl_Net_Ssl_Context_Data *pd)
{
   efl_net_ssl_ctx_teardown(&pd->ssl_ctx);

   _efl_net_ssl_context_string_list_free(&pd->certificates);
   _efl_net_ssl_context_string_list_free(&pd->private_keys);
   _efl_net_ssl_context_string_list_free(&pd->certificate_revocation_lists);
   _efl_net_ssl_context_string_list_free(&pd->certificate_authorities);

   eina_stringshare_replace(&pd->hostname, NULL);

   efl_destructor(efl_super(o, MY_CLASS));
}

/** @brief Global singleton instance for the default SSL dialer context. */
static Efl_Net_Ssl_Context *_efl_net_ssl_context_default_dialer = NULL;

/**
 * @brief Event callback to clear the global default dialer when it's deleted.
 * @internal
 */
static void
_efl_net_ssl_context_default_dialer_del(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   _efl_net_ssl_context_default_dialer = NULL;
}

/**
 * @brief Gets the default SSL context for dialers (clients).
 *
 * This function provides a globally shared SSL context instance configured with
 * common defaults for client connections:
 * - Verification mode: REQUIRED
 * - Hostname verification: ENABLED
 * - Default CA paths: LOADED
 * - Cipher: AUTO
 * - Type: Dialer (client)
 *
 * The context is created on first request and reused for subsequent calls.
 * It is automatically cleaned up when the main loop exits or if explicitly deleted.
 *
 * @return A pointer to the default Efl_Net_Ssl_Context for dialers.
 *         This object should not be manually unref'd by the caller if obtained
 *         through this function, as its lifecycle is managed globally.
 */
EOLIAN static Efl_Net_Ssl_Context *
_efl_net_ssl_context_default_dialer_get(void)
{
   if (!_efl_net_ssl_context_default_dialer)
     {
        _efl_net_ssl_context_default_dialer = efl_add(EFL_NET_SSL_CONTEXT_CLASS, efl_main_loop_get(),
                                                      efl_net_ssl_context_verify_mode_set(efl_added, EFL_NET_SSL_VERIFY_MODE_REQUIRED),
                                                      efl_net_ssl_context_hostname_verify_set(efl_added, EINA_TRUE),
                                                      efl_net_ssl_context_default_paths_load_set(efl_added, EINA_TRUE),
                                                      efl_net_ssl_context_setup(efl_added, EFL_NET_SSL_CIPHER_AUTO, EINA_TRUE));
        efl_event_callback_add(_efl_net_ssl_context_default_dialer,
                               EFL_EVENT_DEL,
                               _efl_net_ssl_context_default_dialer_del,
                               NULL);
     }
   return _efl_net_ssl_context_default_dialer;
}

#include "efl_net_ssl_context.eo.c"
