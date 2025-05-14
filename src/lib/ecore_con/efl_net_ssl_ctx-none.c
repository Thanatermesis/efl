struct _Efl_Net_Ssl_Ctx {
};

/**
 * @internal
 * @brief Creates a new SSL connection context.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * It always returns NULL.
 *
 * @param ctx The SSL context (unused).
 * @return Always NULL.
 */
static void *
efl_net_ssl_ctx_connection_new(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Sets up the SSL context with the given configuration.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * It always returns ENOSYS.
 *
 * @param ctx The SSL context (unused).
 * @param cfg The SSL context configuration (unused).
 * @return Always ENOSYS.
 */
static Eina_Error
efl_net_ssl_ctx_setup(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED, Efl_Net_Ssl_Ctx_Config cfg EINA_UNUSED)
{
   ERR("EFL compiled with --with-crypto=none");
   return ENOSYS;
}

/**
 * @internal
 * @brief Tears down the SSL context.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * This function does nothing.
 *
 * @param ctx The SSL context (unused).
 */
static void
efl_net_ssl_ctx_teardown(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Sets the SSL verification mode.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * It always returns ENOSYS.
 *
 * @param ctx The SSL context (unused).
 * @param verify_mode The verification mode to set (unused).
 * @return Always ENOSYS.
 */
static Eina_Error
efl_net_ssl_ctx_verify_mode_set(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED, Efl_Net_Ssl_Verify_Mode verify_mode EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Sets whether to verify the hostname.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * It always returns ENOSYS.
 *
 * @param ctx The SSL context (unused).
 * @param hostname_verify EINA_TRUE to enable hostname verification, EINA_FALSE otherwise (unused).
 * @return Always ENOSYS.
 */
static Eina_Error
efl_net_ssl_ctx_hostname_verify_set(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED, Eina_Bool hostname_verify EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Sets the hostname for SSL verification.
 *
 * This is a stub implementation because EFL was compiled with --with-crypto=none.
 * It always returns ENOSYS.
 *
 * @param ctx The SSL context (unused).
 * @param hostname The hostname to set (unused).
 * @return Always ENOSYS.
 */
static Eina_Error
efl_net_ssl_ctx_hostname_set(Efl_Net_Ssl_Ctx *ctx EINA_UNUSED, const char *hostname EINA_UNUSED)
{
   return ENOSYS;
}
