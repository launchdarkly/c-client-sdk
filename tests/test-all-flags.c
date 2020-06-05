#include "ldapi.h"
#include "ldinternal.h"

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
testBasicAllFlags()
{
    struct LDClient *client;
    struct LDFlag flag;
    struct LDJSON *expected, *actual;

    LD_ASSERT(client = makeTestClient());

    flag.key                  = LDStrDup("test");
    flag.value                = LDNewText("alice");
    flag.version              = 2;
    flag.variation            = 3;
    flag.trackEvents          = false;
    flag.reason               = NULL;
    flag.debugEventsUntilDate = 0;
    flag.deleted              = false;

    LD_ASSERT(LDi_storeUpsert(&client->store, flag));

    LD_ASSERT(expected = LDJSONDeserialize("{\"test\": \"alice\"}"));

    LD_ASSERT(actual = LDAllFlags(client));

    LD_ASSERT(LDJSONCompare(expected, actual));

    LDJSONFree(expected);
    LDJSONFree(actual);
    LDClientClose(client);
}

static void
testAllFlagsEmpty()
{
    struct LDClient *client;
    struct LDFlag flag;
    struct LDJSON *expected, *actual;

    LD_ASSERT(client = makeTestClient());

    LD_ASSERT(expected = LDNewObject());

    LD_ASSERT(actual = LDAllFlags(client));

    LD_ASSERT(LDJSONCompare(expected, actual));

    LDJSONFree(expected);
    LDJSONFree(actual);
    LDClientClose(client);
}

int
main()
{
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLogger);

    testBasicAllFlags();
    testAllFlagsEmpty();

    return 0;
}
