#include <launchdarkly/api.h>

#include "ldinternal.h"

#include "event_processor.h"

static struct LDClient *
makeTestClient()
{
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("test-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    return client;
}

static void
testNoPayloadIfNoEvents()
{
    struct LDConfig *config;
    struct EventProcessor *processor;
    struct LDJSON *payload;

    LD_ASSERT(config = LDConfigNew("abc"));
    LD_ASSERT(processor = LDi_newEventProcessor(config));

    LD_ASSERT(LDi_bundleEventPayload(processor, &payload));
    LD_ASSERT(payload == NULL);

    LDi_freeEventProcessor(processor);
    LDConfigFree(config);
}

static void
testTrackMetricQueued()
{
    struct LDClient *client;
    double metricValue;
    const char *metricName;
    struct LDJSON *payload, *event;

    metricValue = 0.52;
    metricName = "metricName";

    LD_ASSERT(client = makeTestClient());

    LDClientTrackMetric(client, metricName, NULL, metricValue);

    LD_ASSERT(LDi_bundleEventPayload(client->eventProcessor, &payload))

    LD_ASSERT(LDCollectionGetSize(payload) == 2);
    LD_ASSERT(event = LDArrayLookup(payload, 1));
    LD_ASSERT(strcmp(metricName, LDGetText(LDObjectLookup(event, "key"))) == 0);
    LD_ASSERT(strcmp("custom", LDGetText(LDObjectLookup(event, "kind"))) == 0);
    LD_ASSERT(metricValue == LDGetNumber(LDObjectLookup(event, "metricValue")));

    LDJSONFree(payload);

    LDClientClose(client);
}

static void
testBasicSummary()
{
    struct LDClient *client;
    struct LDFlag flag;
    struct LDJSON *payload, *event, *expected, *startDate, *endDate;
    char *expectedString;

    LD_ASSERT(client = makeTestClient());

    flag.key                  = LDStrDup("test");
    flag.value                = LDNewBool(true);
    flag.version              = 2;
    flag.flagVersion          = -1;
    flag.variation            = 3;
    flag.trackEvents          = true;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(LDi_storeUpsert(&client->store, flag));

    LD_ASSERT(LDBoolVariation(client, "test", false) == true);
    LD_ASSERT(LDBoolVariation(client, "test", false) == true);

    LD_ASSERT(LDi_bundleEventPayload(client->eventProcessor, &payload));
    LD_ASSERT(LDCollectionGetSize(payload) == 4);
    LD_ASSERT(event = LDArrayLookup(payload, 3))

    LD_ASSERT(startDate = LDObjectLookup(event, "startDate"));
    LD_ASSERT(LDJSONGetType(startDate) == LDNumber);
    LD_ASSERT(endDate = LDObjectLookup(event, "endDate"));
    LD_ASSERT(LDJSONGetType(endDate) == LDNumber);
    LD_ASSERT(LDGetNumber(startDate) <= LDGetNumber(endDate));

    LDObjectDeleteKey(event, "startDate");
    LDObjectDeleteKey(event, "endDate");

    LD_ASSERT(expected = LDJSONDeserialize(
        "{\"kind\":\"summary\",\"features\":{\"test\":{\"default\":false,"
        "\"counters\":[{\"count\":2,\"value\":true,\"version\":2,"
        "\"variation\":3}]}}}"
    ));

    LD_ASSERT(LDJSONCompare(event, expected));

    LDJSONFree(expected);
    LDJSONFree(payload);
    LDClientClose(client);
}

static void
testBasicSummaryUnknown()
{
    struct LDClient *client;
    struct LDJSON *payload, *event, *expected, *startDate, *endDate;

    LD_ASSERT(client = makeTestClient());

    LD_ASSERT(LDBoolVariation(client, "test", false) == false);

    LD_ASSERT(LDi_bundleEventPayload(client->eventProcessor, &payload));
    LD_ASSERT(LDCollectionGetSize(payload) == 2);
    LD_ASSERT(event = LDArrayLookup(payload, 1))

    LD_ASSERT(startDate = LDObjectLookup(event, "startDate"));
    LD_ASSERT(LDJSONGetType(startDate) == LDNumber);
    LD_ASSERT(endDate = LDObjectLookup(event, "endDate"));
    LD_ASSERT(LDJSONGetType(endDate) == LDNumber);
    LD_ASSERT(LDGetNumber(startDate) <= LDGetNumber(endDate));

    LDObjectDeleteKey(event, "startDate");
    LDObjectDeleteKey(event, "endDate");

    LD_ASSERT(expected = LDJSONDeserialize(
        "{\"kind\":\"summary\", \"features\":{\"test\":{\"default\":false,"
        "\"counters\":[{\"count\":1,\"value\":false,\"unknown\":true}]}}}"
    ));

    LD_ASSERT(LDJSONCompare(event, expected));

    LDJSONFree(expected);
    LDJSONFree(payload);
    LDClientClose(client);
}

static void
testInlineUser()
{
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;
    struct LDJSON *payload, *event, *expected;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);
    LDConfigSetInlineUsersInEvents(config, true);

    LD_ASSERT(user = LDUserNew("my-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    LDClientTrack(client, "my-metric");

    LD_ASSERT(LDi_bundleEventPayload(client->eventProcessor, &payload));
    LD_ASSERT(LDCollectionGetSize(payload) == 2);
    LD_ASSERT(event = LDArrayLookup(payload, 1))

    LD_ASSERT(expected = LDJSONDeserialize(
        "{\"key\":\"my-user\"}"
    ));

    LD_ASSERT(LDJSONCompare(LDObjectLookup(event, "user"), expected));
    LD_ASSERT(!LDObjectLookup(event, "userKey"));

    LDJSONFree(payload);
    LDJSONFree(expected);
    LDClientClose(client);
}

static void
testOnlyUserKey()
{
    struct LDConfig *config;
    struct LDUser *user;
    struct LDClient *client;
    struct LDJSON *payload, *event, *expected;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("my-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    LDClientTrack(client, "my-metric");

    LD_ASSERT(LDi_bundleEventPayload(client->eventProcessor, &payload));
    LD_ASSERT(LDCollectionGetSize(payload) == 2);
    LD_ASSERT(event = LDArrayLookup(payload, 1))

    LD_ASSERT(expected = LDNewText("my-user"));

    LD_ASSERT(LDJSONCompare(LDObjectLookup(event, "userKey"), expected));
    LD_ASSERT(!LDObjectLookup(event, "user"));

    LDJSONFree(payload);
    LDJSONFree(expected);
    LDClientClose(client);
}

int
main()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);

    testNoPayloadIfNoEvents();
    testTrackMetricQueued();
    testBasicSummary();
    testBasicSummaryUnknown();
    testInlineUser();
    testOnlyUserKey();

    LDBasicLoggerThreadSafeShutdown();

    return 0;
}
