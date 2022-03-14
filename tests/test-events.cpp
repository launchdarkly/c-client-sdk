#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>

#include "ldinternal.h"

#include "event_processor.h"
#include "event_processor_internal.h"
}

// Inherit from the CommonFixture to give a reasonable name for the test output.
// Any custom setup and teardown would happen in this derived class.
class EventsFixture : public CommonFixture {
};

class EventsWithClientFixture : public CommonFixture {
protected:
    struct LDClient *client;

    void SetUp() override {
        CommonFixture::SetUp();

        struct LDConfig *config;
        struct LDUser *user;

        config = LDConfigNew("abc");
        LDConfigSetOffline(config, LDBooleanTrue);

        user = LDUserNew("test-user");

        client = LDClientInit(config, user, 0);
    }

    void TearDown() override {
        LDClientClose(client);
        CommonFixture::TearDown();
    }
};

TEST_F(EventsFixture, NoPayloadIfNoEvents) {
    struct LDConfig *config;
    struct EventProcessor *processor;
    struct LDJSON *payload;

    ASSERT_TRUE(config = LDConfigNew("abc"));
    ASSERT_TRUE(processor = LDi_newEventProcessor(config));

    ASSERT_TRUE(LDi_bundleEventPayload(processor, &payload));
    ASSERT_EQ(payload, nullptr);

    LDi_freeEventProcessor(processor);
    LDConfigFree(config);
}

TEST_F(EventsWithClientFixture, TrackMetricQueued) {
    double metricValue;
    const char *metricName;
    struct LDJSON *payload, *event;

    metricValue = 0.52;
    metricName = "metricName";

    LDClientTrackMetric(client, metricName, NULL, metricValue);

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));

    ASSERT_EQ(LDCollectionGetSize(payload), 2);
    ASSERT_TRUE(event = LDArrayLookup(payload, 1));
    ASSERT_STREQ(metricName, LDGetText(LDObjectLookup(event, "key")));
    ASSERT_STREQ("custom", LDGetText(LDObjectLookup(event, "kind")));
    ASSERT_EQ(metricValue, LDGetNumber(LDObjectLookup(event, "metricValue")));

    LDJSONFree(payload);
}

TEST_F(EventsWithClientFixture, AliasEventIsQueued) {
    double metricValue;
    const char *metricName;
    struct LDJSON *payload, *event;
    struct LDUser *previous, *current;

    ASSERT_TRUE(previous = LDUserNew("p"));
    ASSERT_TRUE(current = LDUserNew("c"));

    LDClientAlias(client, current, previous);

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));

    ASSERT_EQ(LDCollectionGetSize(payload), 2);
    ASSERT_TRUE(event = LDArrayLookup(payload, 1));
    ASSERT_STREQ("alias", LDGetText(LDObjectLookup(event, "kind")));

    LDUserFree(previous);
    LDUserFree(current);
    LDJSONFree(payload);
}

TEST_F(EventsWithClientFixture, BasicSummary) {
    struct LDFlag flag;
    struct LDJSON *payload, *event, *expected, *startDate, *endDate;

    flag.key = LDStrDup("test");
    flag.value = LDNewBool(LDBooleanTrue);
    flag.version = 2;
    flag.flagVersion = -1;
    flag.variation = 3;
    flag.trackEvents = LDBooleanTrue;
    flag.reason = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted = LDBooleanFalse;

    ASSERT_TRUE(LDi_storeUpsert(&client->store, flag));

    ASSERT_TRUE(LDBoolVariation(client, "test", LDBooleanFalse));
    ASSERT_TRUE(LDBoolVariation(client, "test", LDBooleanFalse));

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));
    ASSERT_EQ(LDCollectionGetSize(payload), 4);
    ASSERT_TRUE(event = LDArrayLookup(payload, 3));

    ASSERT_TRUE(startDate = LDObjectLookup(event, "startDate"));
    ASSERT_EQ(LDJSONGetType(startDate), LDNumber);
    ASSERT_TRUE(endDate = LDObjectLookup(event, "endDate"));
    ASSERT_EQ(LDJSONGetType(endDate), LDNumber);
    ASSERT_LE(LDGetNumber(startDate), LDGetNumber(endDate));

    LDObjectDeleteKey(event, "startDate");
    LDObjectDeleteKey(event, "endDate");

    ASSERT_TRUE(
            expected = LDJSONDeserialize(
                    "{\"kind\":\"summary\",\"features\":{\"test\":{\"default\":false,"
                    "\"counters\":[{\"count\":2,\"value\":true,\"version\":2,"
                    "\"variation\":3}]}}}"));

    ASSERT_TRUE(LDJSONCompare(event, expected));

    LDJSONFree(expected);
    LDJSONFree(payload);
}

TEST_F(EventsWithClientFixture, BasicSummaryUnknown) {
    struct LDJSON *payload, *event, *expected, *startDate, *endDate;

    ASSERT_FALSE(LDBoolVariation(client, "test", LDBooleanFalse));

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));
    ASSERT_EQ(LDCollectionGetSize(payload), 2);
    ASSERT_TRUE(event = LDArrayLookup(payload, 1));

    ASSERT_TRUE(startDate = LDObjectLookup(event, "startDate"));
    ASSERT_EQ(LDJSONGetType(startDate), LDNumber);
    ASSERT_TRUE(endDate = LDObjectLookup(event, "endDate"));
    ASSERT_EQ(LDJSONGetType(endDate), LDNumber);
    ASSERT_LE(LDGetNumber(startDate), LDGetNumber(endDate));

    LDObjectDeleteKey(event, "startDate");
    LDObjectDeleteKey(event, "endDate");

    ASSERT_TRUE(
            expected = LDJSONDeserialize(
                    "{\"kind\":\"summary\", \"features\":{\"test\":{\"default\":false,"
                    "\"counters\":[{\"count\":1,\"value\":false,\"unknown\":true}]}}"
                    "}"));

    ASSERT_TRUE(LDJSONCompare(event, expected));

    LDJSONFree(expected);
    LDJSONFree(payload);
}

TEST_F(EventsFixture, InlineUser) {
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;
    struct LDJSON *payload, *event, *expected;

    ASSERT_TRUE(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, LDBooleanTrue);
    LDConfigSetInlineUsersInEvents(config, LDBooleanTrue);

    ASSERT_TRUE(user = LDUserNew("my-user"));

    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    LDClientTrack(client, "my-metric");

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));
    ASSERT_EQ(LDCollectionGetSize(payload), 2);
    ASSERT_TRUE(event = LDArrayLookup(payload, 1));

    ASSERT_TRUE(expected = LDJSONDeserialize("{\"key\":\"my-user\"}"));

    ASSERT_TRUE(LDJSONCompare(LDObjectLookup(event, "user"), expected));
    ASSERT_FALSE(LDObjectLookup(event, "userKey"));

    LDJSONFree(payload);
    LDJSONFree(expected);
    LDClientClose(client);
}

TEST_F(EventsFixture, OnlyUserKey) {
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;
    struct LDJSON *payload, *event, *expected;

    ASSERT_TRUE(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, LDBooleanTrue);

    ASSERT_TRUE(user = LDUserNew("my-user"));

    ASSERT_TRUE(client = LDClientInit(config, user, 0));

    LDClientTrack(client, "my-metric");

    ASSERT_TRUE(LDi_bundleEventPayload(client->eventProcessor, &payload));
    ASSERT_EQ(LDCollectionGetSize(payload), 2);
    ASSERT_TRUE(event = LDArrayLookup(payload, 1));

    ASSERT_TRUE(expected = LDNewText("my-user"));

    ASSERT_TRUE(LDJSONCompare(LDObjectLookup(event, "userKey"), expected));
    ASSERT_FALSE(LDObjectLookup(event, "user"));

    LDJSONFree(payload);
    LDJSONFree(expected);
    LDClientClose(client);
}

TEST_F(EventsFixture, ConstructAliasEvent) {
    struct LDUser *previous, *current;
    struct LDJSON *result, *expected;

    ASSERT_TRUE(previous = LDUserNew("a"));
    ASSERT_TRUE(current = LDUserNew("b"));

    LDUserSetAnonymous(previous, LDBooleanTrue);

    ASSERT_TRUE(result = LDi_newAliasEvent(current, previous, 52));

    ASSERT_TRUE(expected = LDNewObject());
    ASSERT_TRUE(LDObjectSetKey(expected, "kind", LDNewText("alias")));
    ASSERT_TRUE(LDObjectSetKey(expected, "creationDate", LDNewNumber(52)));
    ASSERT_TRUE(LDObjectSetKey(expected, "key", LDNewText("b")));
    ASSERT_TRUE(LDObjectSetKey(expected, "contextKind", LDNewText("user")));
    ASSERT_TRUE(LDObjectSetKey(expected, "previousKey", LDNewText("a")));
    ASSERT_TRUE(LDObjectSetKey(
            expected, "previousContextKind", LDNewText("anonymousUser")));

    ASSERT_TRUE(LDJSONCompare(result, expected));

    LDJSONFree(expected);
    LDJSONFree(result);
    LDUserFree(previous);
    LDUserFree(current);
}
