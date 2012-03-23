
/**
 * Open an HTTP client connection to the given URL
 *
 * @return An int ID used to refer to this connection in
 *  all other methods, or -1 on failure
 */
int
httpc_open(char *url);

/**
 * Read at most count bytes into the given char buffer
 *
 * @return The number of bytes read, or -1 on EOF
 */
int
httpc_read(int conn, char *buffer, int count);

/**
 * Close the connection
 */
void
httpc_close(int conn);
