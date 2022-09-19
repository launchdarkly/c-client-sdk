#include "gtest/gtest.h"
#include "commonfixture.h"
#include <unordered_map>
#include "callback-spy.hpp"

extern "C" {
#include <launchdarkly/api.h>
#include "logging.h"

#include "client.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class ClientFixture : public CommonFixture {};


TEST_F(ClientFixture, ClientMisc) {
    struct LDUser *user;
    struct LDConfig *config;
    struct LDClient *client;

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

DEFINE_STATUS_CALLBACK_2_ARG(withoutUserData);

TEST_F(ClientFixture, CallbackWithoutUserData) {
    struct LDUser *user;
    struct LDConfig *config;
    struct LDClient *client;

    LDSetClientStatusCallback(withoutUserData);

    ASSERT_TRUE(config = LDConfigNew("b"));
    ASSERT_TRUE(user = LDUserNew("a"));
    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    LDClientClose(client);

    auto& calls = STATUS_CALLS(withoutUserData);
    ASSERT_EQ(calls.size(), 2);
    ASSERT_EQ(calls[0].status, LDStatusShuttingdown);
    ASSERT_EQ(calls[1].status, LDStatusFailed);
}

DEFINE_STATUS_CALLBACK_3_ARG(withUserData);

TEST_F(ClientFixture, CallbackWithUserData) {
    struct LDUser *user;
    struct LDConfig *config;
    struct LDClient *client;


    ASSERT_TRUE(config = LDConfigNew("b"));
    ASSERT_TRUE(user = LDUserNew("a"));

    LDSetClientStatusCallbackUserData(withUserData, &client);

    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    LDClientClose(client);

    auto& calls = STATUS_CALLS(withUserData);

    ASSERT_EQ(calls.size(), 2);
    ASSERT_EQ(calls[0].status, LDStatusShuttingdown);
    ASSERT_EQ(calls[0].userData, &client);
    ASSERT_EQ(calls[1].status, LDStatusFailed);
    ASSERT_EQ(calls[1].userData, &client);

}


TEST_F(ClientFixture, SaveFlagsOmitsDeletedFlags) {
    struct LDUser *user;
    struct LDConfig *config;
    struct LDClient *client;
    struct LDFlag flag, flagDeleted;
    char *actual;
    struct LDJSON *expectedJSON, *actualJSON;


    flag.key = LDStrDup("test");
    flag.value = LDNewText("alice");
    flag.version = 2;
    flag.flagVersion = 10;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.trackReason = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    flagDeleted.key = LDStrDup("test2");
    flagDeleted.value = NULL; //Note this is different from a JSON value of null.
    flagDeleted.version = 2;
    flagDeleted.flagVersion = 0;
    flagDeleted.variation = 0;
    flagDeleted.trackEvents = LDBooleanFalse;
    flagDeleted.trackReason = LDBooleanFalse;
    flagDeleted.reason = NULL;
    flagDeleted.debugEventsUntilDate = 0;
    flagDeleted.deleted = LDBooleanTrue;


    ASSERT_TRUE(config = LDConfigNew("b"));
    LDConfigSetOffline(config, LDBooleanTrue);
    ASSERT_TRUE(user = LDUserNew("a"));

    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flagDeleted));

    ASSERT_TRUE(expectedJSON = LDJSONDeserialize("{\"test\":{\"key\":\"test\",\"value\":\"alice\",\"version\":2,\"flagVersion\":10,\"variation\":3}}"));

    ASSERT_TRUE(actual = LDClientSaveFlags(client));
    ASSERT_TRUE(actualJSON = LDJSONDeserialize(actual));

    ASSERT_TRUE(LDJSONCompare(expectedJSON, actualJSON));

    LDClientClose(client);
    LDFree(actual);

    LDJSONFree(expectedJSON);
    LDJSONFree(actualJSON);
}
