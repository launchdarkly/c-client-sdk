#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class HooksFixture : public CommonFixture {
};

static int callCountUpsert = 0;
static int callCountUpsertUserData = 0;
static int intUserData = 123;

void
hookUpsert(const char *const name, const int change) {
    callCountUpsert++;
}

void
hookUpsertUserData(const char *const name, const int change, void *const userData) {
    callCountUpsertUserData++;
    ASSERT_EQ(intUserData, *(int *)userData);
}

TEST_F(HooksFixture, BasicHookUpsert) {
    struct LDFlag flag;
    struct LDStore store;

    ASSERT_TRUE(LDi_storeInitialize(&store));

    ASSERT_TRUE(LDi_storeRegisterListener(&store, "test", hookUpsert));
    ASSERT_TRUE(LDi_storeRegisterListenerUserData(&store, "test", hookUpsertUserData, &intUserData));

    flag.key = LDStrDup("test");
    flag.value = LDNewBool(LDBooleanTrue);
    flag.version = 2;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.trackReason = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    ASSERT_TRUE(flag.key);
    ASSERT_TRUE(flag.value);

    ASSERT_TRUE(LDi_storeUpsert(&store, flag));

    ASSERT_EQ(callCountUpsert, 1);
    ASSERT_EQ(callCountUpsertUserData, 1);

    LDi_storeDestroy(&store);
}

static int callCountPut = 0;
static int callCountPutUserData = 0;

void
hookPut(const char *const name, const int change) {
    callCountPut++;
}

void
hookPutUserData(const char *const name, const int change, void *const userData) {
    callCountPutUserData++;
    ASSERT_EQ(intUserData, *(int *)userData);
}

TEST_F(HooksFixture, BasicHookPut) {
    struct LDFlag *flag;
    struct LDStore store;

    ASSERT_TRUE(flag = static_cast<LDFlag *>(LDAlloc(sizeof(struct LDFlag))));

    ASSERT_TRUE(LDi_storeInitialize(&store));
    ASSERT_TRUE(LDi_storeRegisterListener(&store, "test", hookPut));
    ASSERT_TRUE(LDi_storeRegisterListenerUserData(&store, "test", hookPutUserData, &intUserData));

    flag->key = LDStrDup("test");
    flag->value = LDNewBool(LDBooleanTrue);
    flag->version = 2;
    flag->variation = 3;
    flag->trackEvents = LDBooleanFalse;
    flag->trackReason = LDBooleanFalse;
    flag->reason = NULL;
    flag->debugEventsUntilDate = 0;
    flag->deleted = LDBooleanFalse;

    ASSERT_TRUE(flag->key);
    ASSERT_TRUE(flag->value);

    ASSERT_TRUE(LDi_storePut(&store, flag, 1));

    ASSERT_EQ(callCountPut, 1);
    ASSERT_EQ(callCountPutUserData, 1);

    LDi_storeDestroy(&store);
}
