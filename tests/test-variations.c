#include "ldapi.h"
#include "ldinternal.h"

static LDClient *
makeTestClient()
{
    LDConfig *config;
    LDUser *user;
    LDClient *client;

    LD_ASSERT(config = LDConfigNew("abc"));
    LDConfigSetOffline(config, true);

    LD_ASSERT(user = LDUserNew("test-user"));

    LD_ASSERT(client = LDClientInit(config, user, 0));

    return client;
}

static void
testBoolVariationDefault(LDClient *const client)
{
    LD_ASSERT(LDBoolVariation(client, "test", false) == false);
}

static void
testStringVariationDefault(LDClient *const client)
{
    char buffer[128];

    LD_ASSERT(strcmp(LDStringVariation(
        client, "test", "fallback", buffer, sizeof(buffer)
    ), "fallback") == 0);
}

static void
testStringVariationAllocDefault(LDClient *const client)
{
    char *result;

    result = LDStringVariationAlloc(client, "test", "fallback");

    LD_ASSERT(strcmp(result, "fallback") == 0);

    LDFree(result);
}

static void
testIntVariationDefault(LDClient *const client)
{
    LD_ASSERT(LDIntVariation(client, "test", 3) == 3);
}

static void
testDoubleVariationDefault(LDClient *const client)
{
    LD_ASSERT(LDDoubleVariation(client, "test", 5.3) == 5.3);
}

static void
testJSONVariationDefault(LDClient *const client)
{
    struct LDJSON *fallback, *result;

    LD_ASSERT(fallback = LDNewText("alice"));

    LD_ASSERT(result = LDJSONVariation(client, "test", fallback));

    LD_ASSERT(LDJSONCompare(fallback, result));

    LDJSONFree(fallback);
    LDJSONFree(result);
}

static void
testWithClient(void (*const op)(LDClient *))
{
    LDClient *client;

    LD_ASSERT(op);

    LD_ASSERT(client = makeTestClient());

    op(client);

    LDClientClose(client);
}

static void
testWithClientAndFlagValue(void (*const op)(LDClient *),
    struct LDJSON *const value)
{
    struct LDFlag flag;
    LDClient *client;

    LD_ASSERT(op);
    LD_ASSERT(value);
    LD_ASSERT(client = makeTestClient());

    flag.key                  = LDStrDup("test");
    flag.value                = value;
    flag.version              = 2;
    flag.variation            = 3;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(LDi_storeUpsert(&client->store, flag));

    op(client);

    LDClientClose(client);
}

static void
testBoolVariation(LDClient *const client)
{
    LD_ASSERT(LDBoolVariation(client, "test", false) == true);
}

static void
testIntVariation(LDClient *const client)
{
    LD_ASSERT(LDIntVariation(client, "test", 2) == 3);
}

static void
testDoubleVariation(LDClient *const client)
{
    LD_ASSERT(LDDoubleVariation(client, "test", 2.2) == 3.3);
}

static void
testStringVariation(LDClient *const client)
{
    char buffer[128];

    LD_ASSERT(strcmp(LDStringVariation(
        client, "test", "fallback", buffer, sizeof(buffer)
    ), "value") == 0);
}

static void
testStringVariationAlloc(LDClient *const client)
{
    char *result;

    result = LDStringVariationAlloc(client, "test", "fallback");

    LD_ASSERT(strcmp(result, "value") == 0);

    LDFree(result);
}

static void
testJSONVariation(LDClient *const client)
{
    struct LDJSON *fallback, *result, *expected;

    LD_ASSERT(fallback = LDNewText("fallback"));
    LD_ASSERT(expected = LDNewText("value"));

    LD_ASSERT(result = LDJSONVariation(client, "test", fallback));

    LD_ASSERT(LDJSONCompare(result, expected));

    LDJSONFree(fallback);
    LDJSONFree(expected);
    LDJSONFree(result);
}

static void
testBoolVariationDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDBoolVariationDetail(client, "test", false, details) == false);
}

static void
testIntVariationDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDIntVariationDetail(client, "test", 9, details) == 9);
}

static void
testDoubleVariationDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDDoubleVariationDetail(client, "test", 5.4, details) == 5.4);
}

static void
testStringVariationDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    char buffer[128];

    LD_ASSERT(strcmp(LDStringVariationDetail(
        client, "test", "fallback", buffer, sizeof(buffer), details
    ), "fallback") == 0);
}

static void
testStringVariationAllocDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    char *result;

    LD_ASSERT(result = LDStringVariationAllocDetail(
        client, "test", "fallback", details
    ));

    LD_ASSERT(strcmp(result, "fallback") == 0);

    LDFree(result);
}

static void
testJSONVariationDetailDefault(LDClient *const client,
    LDVariationDetails *const details)
{
    struct LDJSON *result, *fallback;

    LD_ASSERT(fallback = LDNewText("fallback"));

    LD_ASSERT(result = LDJSONVariationDetail(
        client, "test", fallback, details
    ));

    LD_ASSERT(LDJSONCompare(result, fallback));

    LDJSONFree(result);
    LDJSONFree(fallback);
}

static void
testWithClientAndDetails(void (*const op)(LDClient *, LDVariationDetails *))
{
    LDClient *client;
    LDVariationDetails details;

    LD_ASSERT(op);

    LD_ASSERT(client = makeTestClient());

    op(client, &details);

    LDFreeDetailContents(details);
    LDClientClose(client);
}

static void
testBoolVariationDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDBoolVariationDetail(client, "test", false, details) == true);
}

static void
testIntVariationDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDIntVariationDetail(client, "test", 5, details) == 6);
}

static void
testDoubleVariationDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    LD_ASSERT(LDDoubleVariationDetail(client, "test", 5.3, details) == 9.1);
}

static void
testStringVariationDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    char buffer[128];

    LD_ASSERT(strcmp(LDStringVariationDetail(
        client, "test", "fallback", buffer, sizeof(buffer), details
    ), "b") == 0);
}

static void
testStringVariationAllocDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    char *result;

    LD_ASSERT(result = LDStringVariationAllocDetail(
        client, "test", "fallback", details
    ));

    LD_ASSERT(strcmp(result, "b") == 0);

    LDFree(result);
}

static void
testJSONVariationDetail(LDClient *const client,
    LDVariationDetails *const details)
{
    struct LDJSON *result, *expected, *fallback;

    LD_ASSERT(fallback = LDNewText("fallback"));

    LD_ASSERT(result = LDJSONVariationDetail(
        client, "test", fallback, details
    ));

    LD_ASSERT(expected = LDNewText("c"))

    LD_ASSERT(LDJSONCompare(result, expected));

    LDJSONFree(result);
    LDJSONFree(expected);
    LDJSONFree(fallback);
}

static void
testWithClientAndDetailsAndValue(
    void (*const op)(LDClient *, LDVariationDetails *),
    struct LDJSON *const value
) {
    LDClient *client;
    LDVariationDetails details;
    struct LDFlag flag;

    LD_ASSERT(op);

    LD_ASSERT(client = makeTestClient());

    flag.key                  = LDStrDup("test");
    flag.value                = value;
    flag.version              = 2;
    flag.variation            = 3;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(LDi_storeUpsert(&client->store, flag));

    op(client, &details);

    LDFreeDetailContents(details);
    LDClientClose(client);
}

int
main()
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    testWithClient(testBoolVariationDefault);
    testWithClient(testIntVariationDefault);
    testWithClient(testDoubleVariationDefault);
    testWithClient(testStringVariationDefault);
    testWithClient(testStringVariationAllocDefault);
    testWithClient(testJSONVariationDefault);

    testWithClientAndFlagValue(testBoolVariation, LDNewBool(true));
    testWithClientAndFlagValue(testIntVariation, LDNewNumber(3));
    testWithClientAndFlagValue(testDoubleVariation, LDNewNumber(3.3));
    testWithClientAndFlagValue(testStringVariation, LDNewText("value"));
    testWithClientAndFlagValue(testStringVariationAlloc, LDNewText("value"));
    testWithClientAndFlagValue(testJSONVariation, LDNewText("value"));

    testWithClientAndDetails(testBoolVariationDetailDefault);
    testWithClientAndDetails(testIntVariationDetailDefault);
    testWithClientAndDetails(testDoubleVariationDetailDefault);
    testWithClientAndDetails(testStringVariationDetailDefault);
    testWithClientAndDetails(testStringVariationAllocDetailDefault);
    testWithClientAndDetails(testJSONVariationDetailDefault);

    testWithClientAndDetailsAndValue(testBoolVariationDetail, LDNewBool(true));
    testWithClientAndDetailsAndValue(testIntVariationDetail, LDNewNumber(6));
    testWithClientAndDetailsAndValue(testDoubleVariationDetail,
        LDNewNumber(9.1));
    testWithClientAndDetailsAndValue(testStringVariationDetail, LDNewText("b"));
    testWithClientAndDetailsAndValue(testStringVariationAllocDetail,
        LDNewText("b"));
    testWithClientAndDetailsAndValue(testJSONVariationDetail, LDNewText("c"));

    return 0;
}
