#include "gtest/gtest.h"
#include "commonfixture.h"
#include <unordered_map>

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}


// Note: Using global callbacks + variables for the following tests is not ideal.
// It would be better to pass this type of context as a userData parameter in the listeners, but that isn't
// currently how the callbacks are designed.

// Holds the data received by callback invocation, of the form (flagKey, status).
struct call {
    std::string flag;
    int status;
    call(std::string flag, int status): flag(std::move(flag)), status(status) {}
};

struct callbackSpy {
    using Data = std::vector<call>;
    std::unordered_map<const char*, Data> cbs;

    // Record a callback for a unit test name. Meant to be called from within
    // a registered callback.
    void record(const char* test, const char* flagKey, int status) {
        cbs[test].emplace_back(flagKey, status);
    }

    // Retrieve the callback data associated with a given unit test.
    const Data& test(const char* test) {
        return cbs[test];
    }
};

// Helper to define a callback function for a particular unit test.
#define DEFINE_TEST_CALLBACK(name) \
static void name(const char* const flagKey, const int status) { spy.record(#name, flagKey, status); }

static callbackSpy spy;

// Used for unit testing the ChangeListener implementation detail.
class ChangeListenerFixture : public CommonFixture {};

TEST_F(ChangeListenerFixture, TestInitFreeDoesNotLeak) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);
    LDi_freeListeners(&listeners);
}

TEST_F(ChangeListenerFixture, TestInsertWithoutDeleteDoesNotLeak) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);
    LDi_listenerAdd(&listeners, "flag1", nullptr);
    LDi_freeListeners(&listeners);
}

TEST_F(ChangeListenerFixture, TestDeleteNoop) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);
    LDi_listenerRemove(&listeners, "flag1", nullptr);
    LDi_freeListeners(&listeners);
}

DEFINE_TEST_CALLBACK(testDispatchAfterInsert);

TEST_F(ChangeListenerFixture, TestDispatchAfterInsert) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);

    LDi_listenerAdd(&listeners, "flag1", testDispatchAfterInsert);
    LDi_listenersDispatch(listeners, "flag1", 0);

    LDi_freeListeners(&listeners);

    auto& calls = spy.test("testDispatchAfterInsert");

    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls.at(0).flag, "flag1");
    ASSERT_EQ(calls.at(0).status, 0);
}

DEFINE_TEST_CALLBACK(testDispatchAfterDelete);

TEST_F(ChangeListenerFixture, TestDispatchNoopAfterDelete) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);

    LDi_listenerAdd(&listeners, "flag1", testDispatchAfterDelete);
    LDi_listenerRemove(&listeners, "flag1", testDispatchAfterDelete);

    LDi_listenersDispatch(listeners, "flag1", 0);
    LDi_freeListeners(&listeners);

    ASSERT_TRUE(spy.test("testDispatchAfterDelete").empty());
}

DEFINE_TEST_CALLBACK(testMultiDispatch1);
DEFINE_TEST_CALLBACK(testMultiDispatch2);

TEST_F(ChangeListenerFixture, TestMultiDispatch) {
    struct ChangeListener *listeners;
    LDi_initListeners(&listeners);

    LDi_listenerAdd(&listeners, "flag1", testMultiDispatch1);
    LDi_listenerAdd(&listeners, "flag1", testMultiDispatch2);

    LDi_listenersDispatch(listeners, "flag1", 0);
    LDi_freeListeners(&listeners);

    ASSERT_EQ(spy.test("testMultiDispatch1").size(), 1);
    ASSERT_EQ(spy.test("testMultiDispatch2").size(), 1);
}

// Used for testing the higher-level LDRegister/Unregister listener API surface.
class FlagListenerFixture : public CommonFixture {
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

static LDFlag makeFlag(const char* name) {
    struct LDFlag flag;
    flag.key = LDStrDup(name);
    flag.value = LDNewBool(LDBooleanTrue);
    flag.version = 2;
    flag.flagVersion = -1;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;
    return flag;
}

// This test should generate valgrind errors if the client's Close routine doesn't call the listener
// list destructor.
TEST_F(FlagListenerFixture, TestMemoryFreedByClientClose) {
    LDClientRegisterFeatureFlagListener(client, "flag1", NULL);
}

DEFINE_TEST_CALLBACK(listenerAdded)

TEST_F(FlagListenerFixture, TestListenerAddedReceivesCallbacks) {
    LDClientRegisterFeatureFlagListener(client, "flag1", listenerAdded);

    LDFlag flag = makeFlag("flag1");

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_TRUE(LDi_storeDelete(&client->store, flag.key, flag.version));

    LDClientUnregisterFeatureFlagListener(client, "flag1", listenerAdded);

    auto& calls = spy.test("listenerAdded");

    // Upsert and delete flag should result in two callback invocations: one for addition
    // of the flag, and the other for deletion.

    ASSERT_EQ(calls.size(), 2);
    EXPECT_EQ(calls.at(0).flag, "flag1");
    ASSERT_EQ(calls.at(0).status, 0);
    EXPECT_EQ(calls.at(1).flag, "flag1");
    ASSERT_EQ(calls.at(1).status, 1);

}

DEFINE_TEST_CALLBACK(listenerRemoved);

TEST_F(FlagListenerFixture, TestListenerRemovedIgnoresCallback) {
    LDClientRegisterFeatureFlagListener(client, "flag1", listenerRemoved);

    LDFlag flag = makeFlag("flag1");
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    LDClientUnregisterFeatureFlagListener(client, "flag1", listenerRemoved);

    ASSERT_TRUE(LDi_storeDelete(&client->store, "flag1", 2));

    auto& calls = spy.test("listenerRemoved");

    // Since the listener was removed before the storeDelete, it should only have received
    // one callback.
    ASSERT_EQ(calls.size(), 1);
    EXPECT_EQ(calls.at(0).flag, "flag1");
    ASSERT_EQ(calls.at(0).status, 0);

}

DEFINE_TEST_CALLBACK(enforceUniqueness);

// Tests that a particular callback can only be registered for a particular flag once.
TEST_F(FlagListenerFixture, TestListenerUniqueness) {

    ASSERT_TRUE(LDClientRegisterFeatureFlagListener(client, "flag1", enforceUniqueness));
    ASSERT_TRUE(LDClientRegisterFeatureFlagListener(client, "flag1", enforceUniqueness));
    ASSERT_TRUE(LDClientRegisterFeatureFlagListener(client, "flag1", enforceUniqueness));

    ASSERT_TRUE(LDi_storeUpsert(&client->store, makeFlag("flag1")));

    auto& calls = spy.test("enforceUniqueness");

    ASSERT_EQ(calls.size(), 1);
}
