/**
 * @internal
 * @brief Opaque structure representing an SSL connection.
 *
 * In this "none" implementation (when EFL is compiled with --with-crypto=none),
 * this structure is empty as no actual SSL operations are performed.
 */
struct _Efl_Net_Ssl_Conn {
};

/**
 * @internal
 * @brief Sets up an SSL connection.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to initialize SSL/TLS for the given socket, either as a
 * client (dialer) or server.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param is_dialer EINA_TRUE if this is a client connection, EINA_FALSE for server (unused).
 * @param sock The underlying network socket (unused).
 * @param context The SSL context to use (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_setup(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Eina_Bool is_dialer EINA_UNUSED, Efl_Net_Socket *sock EINA_UNUSED, Efl_Net_Ssl_Context *context EINA_UNUSED)
{
   ERR("EFL compiled with --with-crypto=none");
   return ENOSYS;
}

/**
 * @internal
 * @brief Tears down an SSL connection.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to clean up resources associated with an SSL connection.
 *
 * @param conn The SSL connection object (unused in this implementation).
 */
static void
efl_net_ssl_conn_teardown(Efl_Net_Ssl_Conn *conn EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Writes data to an SSL connection.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to encrypt and send the provided data slice over the
 * SSL/TLS connection.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param slice The data slice to write (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_write(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Eina_Slice *slice EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Reads data from an SSL connection.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to read and decrypt data from the SSL/TLS connection
 * into the provided buffer slice.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param slice The read-write slice to store the received data (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_read(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Eina_Rw_Slice *slice EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Performs the SSL handshake.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to execute the SSL/TLS handshake process.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param done Pointer to a boolean that will be set to EINA_TRUE if the
 *             handshake is complete, EINA_FALSE otherwise (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_handshake(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Eina_Bool *done EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Sets the peer verification mode for an SSL connection.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to configure how the SSL peer's certificate is verified.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param verify_mode The verification mode to set (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_verify_mode_set(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Efl_Net_Ssl_Verify_Mode verify_mode EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Sets whether to verify the peer's hostname against its certificate.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to enable or disable hostname verification during the
 * SSL/TLS handshake.
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param hostname_verify EINA_TRUE to enable hostname verification,
 *                        EINA_FALSE to disable (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_hostname_verify_set(Efl_Net_Ssl_Conn *conn EINA_UNUSED, Eina_Bool hostname_verify EINA_UNUSED)
{
   return ENOSYS;
}

/**
 * @internal
 * @brief Overrides the hostname used for peer certificate verification.
 *
 * This function is a stub when EFL is compiled with --with-crypto=none.
 * It is intended to allow specifying a different hostname for verification
 * than the one used to establish the connection (e.g., for virtual hosting).
 *
 * @param conn The SSL connection object (unused in this implementation).
 * @param hostname The hostname to use for verification (unused).
 * @return Always returns ENOSYS, indicating the function is not implemented
 *         due to the lack of a crypto backend.
 */
static Eina_Error
efl_net_ssl_conn_hostname_override_set(Efl_Net_Ssl_Conn *conn EINA_UNUSED, const char *hostname EINA_UNUSED)
{
   return ENOSYS;
}
