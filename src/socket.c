#include "socket.h"

enum status {
    UNINITIALIZED = 0,
    OPEN = 1,
    CLOSED = 2
};

void
LDi_initSocket(struct ld_socket_state *socket) {
    socket->status = UNINITIALIZED;
}

LDBoolean
LDi_socketLoad(const struct ld_socket_state *const socket, int *out_fd) {
    *out_fd = socket->fd;
    return (LDBoolean) (socket->status == OPEN);
}

void
LDi_socketStore(struct ld_socket_state *socket, int fd) {
    socket->fd = fd;
    socket->status = OPEN;
}

void
LDi_socketClose(struct ld_socket_state *socket) {
    if (socket->status == OPEN) {
        socket->status = CLOSED;
    }
}

LDBoolean
LDi_socketClosed(const struct ld_socket_state *socket) {
    return (LDBoolean) (socket->status == CLOSED);
}
