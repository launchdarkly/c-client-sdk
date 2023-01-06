#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}


// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class SSEFixture : public CommonFixture {
protected:
    struct LDClient *client;

    void SetUp() override {
        CommonFixture::SetUp();

        struct LDConfig *config;
        struct LDUser *user;

        LD_ASSERT(config = LDConfigNew("abc"));
        LDConfigSetOffline(config, LDBooleanTrue);

        LD_ASSERT(user = LDUserNew("test-user"));

        LD_ASSERT(client = LDClientInit(config, user, 0));
    }

    void TearDown() override {
        LDClientClose(client);
        CommonFixture::TearDown();
    }
};

TEST_F(SSEFixture, InitialPut_EmptyObject_ShouldResultInitialized) {
    ASSERT_TRUE(LDi_onstreameventput(client, "{}"));
    ASSERT_EQ(client->status, LDStatusInitialized);
}

TEST_F(SSEFixture, InitialPut_MalformedData_EmptyString_ShouldRemainInitializing) {
    ASSERT_FALSE(LDi_onstreameventput(client, ""));
    ASSERT_EQ(client->status, LDStatusInitializing);
}

TEST_F(SSEFixture, InitialPut_MalformedData_InvalidObject_ShouldRemainInitializing) {
    ASSERT_FALSE(LDi_onstreameventput(client, "{\"things\":{}}"));
    ASSERT_EQ(client->status, LDStatusInitializing);
}
