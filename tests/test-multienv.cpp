#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class MultiEnvFixture : public CommonFixture {
};

TEST_F(MultiEnvFixture, MultiEnvTest) {
    struct LDClient *client, *secondary;
    struct LDConfig *config;
    struct LDUser *user;

    ASSERT_TRUE(config = LDConfigNew("my-primary-key"));
    LDConfigSetOffline(config, LDBooleanTrue);
    ASSERT_TRUE(
            LDConfigAddSecondaryMobileKey(config, "secondary", "my-secondary-key"));
    ASSERT_TRUE(user = LDUserNew("my-user"));

    ASSERT_TRUE(client = LDClientInit(config, user, 0));
    ASSERT_TRUE(secondary = LDClientGetForMobileKey("secondary"));
    ASSERT_NE(client, secondary);
    ASSERT_FALSE(LDClientGetForMobileKey("unknown environment"));

    LDClientClose(client);
}
