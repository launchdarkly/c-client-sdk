#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "client.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class ClientFixture : public CommonFixture {
};

void
callback(int status) {
}

TEST_F(ClientFixture, ClientMisc) {
    struct LDUser *user;
    struct LDConfig *config;
    struct LDClient *client;

    LDSetClientStatusCallback(callback);

    ASSERT_TRUE(user = LDUserNew("a"));
    ASSERT_TRUE(config = LDConfigNew("b"));
    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    ASSERT_FALSE(LDClientIsInitialized(client));

    ASSERT_EQ(client, LDClientGet());

    LDClientSetBackground(client, LDBooleanTrue);
    LDClientSetBackground(client, LDBooleanFalse);

    ASSERT_FALSE(client->offline);
    ASSERT_FALSE(LDClientIsOffline(client));
    LDClientSetOffline(client);
    ASSERT_TRUE(LDClientIsOffline(client));
    ASSERT_TRUE(client->offline);
    LDClientSetOnline(client);
    ASSERT_FALSE(LDClientIsOffline(client));
    ASSERT_FALSE(client->offline);

    LDClientClose(client);
}
