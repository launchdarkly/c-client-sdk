#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class AllFlagsWithClientFixture : public CommonFixture {
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

TEST_F(AllFlagsWithClientFixture, BasicAllFlags) {
    struct LDFlag flag;
    struct LDJSON *expected, *actual;

    flag.key = LDStrDup("test");
    flag.value = LDNewText("alice");
    flag.version = 2;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.trackReason = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_TRUE(expected = LDJSONDeserialize("{\"test\": \"alice\"}"));

    ASSERT_TRUE(actual = LDAllFlags(client));

    ASSERT_TRUE(LDJSONCompare(expected, actual));

    LDJSONFree(expected);
    LDJSONFree(actual);
}

TEST_F(AllFlagsWithClientFixture, AllFlagsEmpty) {
    struct LDJSON *expected, *actual;

    ASSERT_TRUE(expected = LDNewObject());

    ASSERT_TRUE(actual = LDAllFlags(client));

    ASSERT_TRUE(LDJSONCompare(expected, actual));

    LDJSONFree(expected);
    LDJSONFree(actual);
}

TEST_F(AllFlagsWithClientFixture, AllFlagsDeletedFlag) {
    struct LDFlag flag;
    struct LDFlag flagDeleted;
    struct LDJSON *expected, *actual;

    flag.key = LDStrDup("test");
    flag.value = LDNewText("alice");
    flag.version = 2;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.trackReason = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    flagDeleted.key = LDStrDup("test2");
    flagDeleted.value = NULL; //Note this is different than a JSON value of null.
    flagDeleted.version = 2;
    flagDeleted.variation = 0;
    flagDeleted.trackEvents = LDBooleanFalse;
    flagDeleted.trackReason = LDBooleanFalse;
    flagDeleted.reason = NULL;
    flagDeleted.debugEventsUntilDate = 0;
    flagDeleted.deleted = true;

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flagDeleted));

    ASSERT_TRUE(expected = LDJSONDeserialize("{\"test\": \"alice\"}"));

    ASSERT_TRUE(actual = LDAllFlags(client));

    ASSERT_TRUE(LDJSONCompare(expected, actual));

    LDJSONFree(expected);
    LDJSONFree(actual);
}
