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

// When receiving a put event, the SDK will allocate an array of LDFlags and fill them in one-by-one
// from JSON payload. If decoding fails, we must ensure that all memory of the previously parsed flag(s)
// is freed. This test requires valgrind.
TEST_F(SSEFixture, InitialPut_MalformedData_AllMemoryIsFreedIfInvalidFlagEncountered) {
    ASSERT_FALSE(LDi_onstreameventput(client, "{\"valid_flag_json\":{\"key\":\"valid_flag\",\"value\":true,\"version\":2,\"variation\":3},\"invalid_flag_json\":{}}"));
}
