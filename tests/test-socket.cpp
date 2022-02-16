#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "socket.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class SocketFixture : public CommonFixture {
};

TEST_F(SocketFixture, UnassignedSocketIsNotClosedByDefault) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    ASSERT_FALSE(LDi_socketClosed(&socket));
}


TEST_F(SocketFixture, AssignedSocketIsNotClosedByDefault) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    LDi_socketStore(&socket, 10);
    ASSERT_FALSE(LDi_socketClosed(&socket));
}

TEST_F(SocketFixture, AssignedSocketIsClosed) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    LDi_socketStore(&socket, 10);
    LDi_socketClose(&socket);
    ASSERT_TRUE(LDi_socketClosed(&socket));
}


TEST_F(SocketFixture, UnassignedSocketIsNotClosed) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    LDi_socketClose(&socket);
    ASSERT_FALSE(LDi_socketClosed(&socket));
}


TEST_F(SocketFixture, UnassignedSocketLoadFails) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    int fd;
    ASSERT_FALSE(LDi_socketLoad(&socket, &fd));
}

TEST_F(SocketFixture, AssignedSocketLoadSucceeds) {
    struct ld_socket_state socket;
    LDi_initSocket(&socket);

    LDi_socketStore(&socket, 10);
    int fd;
    ASSERT_TRUE(LDi_socketLoad(&socket, &fd));
    ASSERT_EQ(fd, 10);
}

TEST_F(SocketFixture, SocketMultipleStore) {
    struct ld_socket_state socket;
    int fd;
    LDi_initSocket(&socket);


    LDi_socketStore(&socket, 1);
    ASSERT_TRUE(LDi_socketLoad(&socket, &fd));
    ASSERT_EQ(1, fd);
    ASSERT_FALSE(LDi_socketClosed(&socket));

    LDi_socketStore(&socket, 2);
    ASSERT_TRUE(LDi_socketLoad(&socket, &fd));
    ASSERT_EQ(2, fd);
    ASSERT_FALSE(LDi_socketClosed(&socket));

    LDi_socketClose(&socket);
    ASSERT_TRUE(LDi_socketClosed(&socket));
}
