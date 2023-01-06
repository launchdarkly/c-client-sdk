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
    flag.trackReason = LDBooleanFalse;
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

TEST_F(StoreFixture, EmptyDataSetShouldNotCauseAbort) {
    struct LDFlag *flags;
    struct LDJSON *json;
    char *jsonStr;
    struct LDStoreNode **storeNodes;
    unsigned int nodeCount;

    // The zero-length allocation simulates the arguments that the SDK would use if
    // it encountered data: {}.
    flags = (struct LDFlag *) LDAlloc(0);
    ASSERT_TRUE(LDi_storePut(&client->store, flags, 0));

    // Get should fail without causing any abort.
    ASSERT_FALSE(LDi_storeGet(&client->store, "key"));

    // The JSON representation should be an empty object.
    ASSERT_TRUE(json = LDi_storeGetJSON(&client->store));
    ASSERT_TRUE(jsonStr = LDJSONSerialize(json));
    ASSERT_STREQ(jsonStr, "{}");

    // GetAll should return a NULL pointer and 0 node count.
    ASSERT_TRUE(LDi_storeGetAll(&client->store, &storeNodes, &nodeCount));
    ASSERT_EQ(nodeCount, 0);
    ASSERT_FALSE(*storeNodes);

    LDJSONFree(json);
    LDFree(jsonStr);
}
