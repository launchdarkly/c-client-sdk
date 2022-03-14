#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class VariationsFixture : public CommonFixture {
};

class VariationsWithClientFixture : public CommonFixture {
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

class VariationsWithClientAndDetail : public VariationsWithClientFixture {
protected:
    LDVariationDetails details;

    void SetUp() override {
        VariationsWithClientFixture::SetUp();
    }

    void TearDown() override {
        LDFreeDetailContents(details);
        VariationsWithClientFixture::TearDown();
    }
};

TEST_F(VariationsWithClientFixture, BoolVariationDefault) {
    ASSERT_FALSE(LDBoolVariation(client, "test", LDBooleanFalse));
}


TEST_F(VariationsWithClientFixture, StringVariationDefault) {
    char buffer[128];

    ASSERT_STREQ(LDStringVariation(client, "test", "fallback", buffer, sizeof(buffer)), "fallback");
}

TEST_F(VariationsWithClientFixture, StringVariationDefaultSmallBuffer) {
    char buffer[3];

    ASSERT_STREQ(LDStringVariation(client, "test", "fallback", buffer, sizeof(buffer)), "fa");
}

TEST_F(VariationsWithClientFixture, VariationAllocDefault) {
    char *result;

    result = LDStringVariationAlloc(client, "test", "fallback");

    ASSERT_STREQ(result, "fallback");

    LDFree(result);
}

TEST_F(VariationsWithClientFixture, IntVariationDefault) {
    ASSERT_EQ(LDIntVariation(client, "test", 3), 3);
}

TEST_F(VariationsWithClientFixture, DoubleVariationDefault) {
    ASSERT_EQ(LDDoubleVariation(client, "test", 5.3), 5.3);
}

TEST_F(VariationsWithClientFixture, JSONVariationDefault) {
    struct LDJSON *fallback, *result;

    ASSERT_TRUE(fallback = LDNewText("alice"));

    ASSERT_TRUE(result = LDJSONVariation(client, "test", fallback));

    ASSERT_TRUE(LDJSONCompare(fallback, result));

    LDJSONFree(fallback);
    LDJSONFree(result);
}

static LDFlag &fillFlag(LDJSON *const value, LDFlag &flag) {
    flag.key = LDStrDup("test");
    flag.value = value;
    flag.version = 2;
    flag.flagVersion = -1;
    flag.variation = 3;
    flag.trackEvents = LDBooleanFalse;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;
    return flag;
}

TEST_F(VariationsWithClientFixture, BoolVariation) {
    struct LDFlag flag;
    fillFlag(LDNewBool(LDBooleanTrue), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_TRUE(LDBoolVariation(client, "test", LDBooleanFalse));
}

TEST_F(VariationsWithClientFixture, IntVariation) {
    struct LDFlag flag;
    fillFlag(LDNewNumber(3), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_EQ(LDIntVariation(client, "test", 2), 3);
}

TEST_F(VariationsWithClientFixture, DoubleVariation) {
    struct LDFlag flag;
    fillFlag(LDNewNumber(3.3), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));
    ASSERT_EQ(LDDoubleVariation(client, "test", 2.2), 3.3);
}

TEST_F(VariationsWithClientFixture, StringVariation) {
    struct LDFlag flag;
    fillFlag(LDNewText("value"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    char buffer[128];

    ASSERT_STREQ(LDStringVariation(client, "test", "fallback", buffer, sizeof(buffer)), "value");
}

TEST_F(VariationsWithClientFixture, StringVariationAlloc) {
    struct LDFlag flag;
    fillFlag(LDNewText("value"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    char *result;

    result = LDStringVariationAlloc(client, "test", "fallback");

    ASSERT_STREQ(result, "value");

    LDFree(result);
}

TEST_F(VariationsWithClientFixture, JSONVariation) {
    struct LDFlag flag;
    fillFlag(LDNewText("value"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    struct LDJSON *fallback, *result, *expected;

    ASSERT_TRUE(fallback = LDNewText("fallback"));
    ASSERT_TRUE(expected = LDNewText("value"));

    ASSERT_TRUE(result = LDJSONVariation(client, "test", fallback));

    ASSERT_TRUE(LDJSONCompare(result, expected));

    LDJSONFree(fallback);
    LDJSONFree(expected);
    LDJSONFree(result);
}

TEST_F(VariationsWithClientAndDetail, BoolVariationDetailDefault) {
    ASSERT_FALSE(LDBoolVariationDetail(client, "test", LDBooleanFalse, &details));
}

TEST_F(VariationsWithClientAndDetail, IntVariationDetailDefault) {
    ASSERT_EQ(LDIntVariationDetail(client, "test", 9, &details), 9);
}

TEST_F(VariationsWithClientAndDetail, DoubleVariationDetailDefault) {
    ASSERT_EQ(LDDoubleVariationDetail(client, "test", 5.4, &details), 5.4);
}

TEST_F(VariationsWithClientAndDetail, StringVariationDetailDefault) {
    char buffer[128];

    ASSERT_STREQ(LDStringVariationDetail(client, "test", "fallback", buffer, sizeof(buffer), &details),
                 "fallback");
}

TEST_F(VariationsWithClientAndDetail, StringVariationDetailDefaultSmallBuffer) {
    char buffer[3];

    ASSERT_STREQ(LDStringVariationDetail(client, "test", "fallback", buffer, sizeof(buffer), &details), "fa");
}

TEST_F(VariationsWithClientAndDetail, StringVariationAllocDetailDefault) {
    char *result;

    ASSERT_TRUE(result =
                        LDStringVariationAllocDetail(client, "test", "fallback", &details));

    ASSERT_STREQ(result, "fallback");

    LDFree(result);
}

TEST_F(VariationsWithClientAndDetail, JSONVariationDetailDefault) {
    struct LDJSON *result, *fallback;

    ASSERT_TRUE(fallback = LDNewText("fallback"));

    ASSERT_TRUE(result = LDJSONVariationDetail(client, "test", fallback, &details));

    ASSERT_TRUE(LDJSONCompare(result, fallback));

    LDJSONFree(result);
    LDJSONFree(fallback);
}

TEST_F(VariationsWithClientAndDetail, BoolVariationDetail) {
    struct LDFlag flag;
    fillFlag(LDNewBool(LDBooleanTrue), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_TRUE(LDBoolVariationDetail(client, "test", LDBooleanFalse, &details));
}

TEST_F(VariationsWithClientAndDetail, IntVariationDetail) {
    struct LDFlag flag;
    fillFlag(LDNewNumber(6), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_EQ(LDIntVariationDetail(client, "test", 5, &details), 6);
}

TEST_F(VariationsWithClientAndDetail, DoubleVariationDetail) {
    struct LDFlag flag;
    fillFlag(LDNewNumber(9.1), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_EQ(LDDoubleVariationDetail(client, "test", 5.3, &details), 9.1);
}

TEST_F(VariationsWithClientAndDetail, StringVariationDetail) {
    struct LDFlag flag;
    fillFlag(LDNewText("b"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    char buffer[128];

    ASSERT_STREQ(LDStringVariationDetail(client, "test", "fallback", buffer, sizeof(buffer), &details), "b");
}

TEST_F(VariationsWithClientAndDetail, StringVariationAllocDetail) {
    struct LDFlag flag;
    fillFlag(LDNewText("b"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    char *result;

    ASSERT_TRUE(result = LDStringVariationAllocDetail(client, "test", "fallback", &details));

    ASSERT_STREQ(result, "b");

    LDFree(result);
}

TEST_F(VariationsWithClientAndDetail, JSONVariationDetail) {
    struct LDFlag flag;
    fillFlag(LDNewText("c"), flag);
    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    struct LDJSON *result, *expected, *fallback;

    ASSERT_TRUE(fallback = LDNewText("fallback"));

    ASSERT_TRUE(result = LDJSONVariationDetail(client, "test", fallback, &details));

    LD_ASSERT(expected = LDNewText("c"))

    LD_ASSERT(LDJSONCompare(result, expected));

    LDJSONFree(result);
    LDJSONFree(expected);
    LDJSONFree(fallback);
}
