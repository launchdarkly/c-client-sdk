#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class StoreFixture : public CommonFixture {
protected:
    struct LDClient *client;

    void SetUp() override {
        struct LDConfig *config;
        struct LDUser *user;

        LD_ASSERT(config = LDConfigNew("abc"));
        LDConfigSetOffline(config, LDBooleanTrue);

        LD_ASSERT(user = LDUserNew("test-user"));

        LD_ASSERT(client = LDClientInit(config, user, 0));
        CommonFixture::SetUp();
    }

    void TearDown() override {
        LDClientClose(client);
        CommonFixture::TearDown();
    }
};

TEST_F(StoreFixture, RestoreAndSaveEmpty) {
    char *bundle;

    ASSERT_TRUE(bundle = LDClientSaveFlags(client));
    ASSERT_TRUE(LDClientRestoreFlags(client, bundle));

    LDFree(bundle);
}

TEST_F(StoreFixture, RestoreAndSaveBasic) {
    char *bundle1, *bundle2;
    struct LDFlag flag;

    flag.key = LDStrDup("test");
    flag.value = LDNewBool(LDBooleanTrue);
    flag.version = 2;
    flag.flagVersion = -1;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_TRUE(bundle1 = LDClientSaveFlags(client));

    LDi_storeFreeFlags(&client->store);

    ASSERT_TRUE(LDClientRestoreFlags(client, bundle1));

    ASSERT_TRUE(bundle2 = LDClientSaveFlags(client));

    ASSERT_STREQ(bundle1, bundle2);

    LDFree(bundle1);
    LDFree(bundle2);
}
