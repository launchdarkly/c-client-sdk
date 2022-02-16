#pragma once

#include <launchdarkly/boolean.h>

/* Represents the state of a non-owned socket. The socket can be unset (before any calls to LDi_socketStore),
 * set (after calls to LDi_socketStore), and closed (after calls to LDi_socketClose). */
struct ld_socket_state {
    int status;
    int fd;
};

/* Initialize an unset socket. Must be called before any other operation. */
void
LDi_initSocket(struct ld_socket_state *socket);

/* Loads the socket's file descriptor into out_fd, if set before with LDi_socketStore. Returns false if unset. */
LDBoolean
LDi_socketLoad(const struct ld_socket_state *socket, int *out_fd);

/* Stores a socket file descriptor. */
void
LDi_socketStore(struct ld_socket_state *socket, int fd);

/* Flags the socket as closed if it was previously open, such that LDi_socketClosed will return true on subsequent calls.
 * Does not perform any actual IO. */
void
LDi_socketClose(struct ld_socket_state *socket);

/* Returns true if the socket was flagged as closed. Socket must have been previously set with LDi_socketStore;
 * an unset socket will not return true. */
LDBoolean
LDi_socketClosed(const struct ld_socket_state *socket);
