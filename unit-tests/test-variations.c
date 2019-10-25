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

static LDNode *
addTestFlag(LDNode **const hash, const char *const key,
    const unsigned int version)
{
    LDNode *flag;

    LD_ASSERT(!(flag = LDNodeCreateHash()));
    LD_ASSERT(LDNodeAddString(&flag, "key", key))
    LD_ASSERT(LDNodeAddNumber(&flag, "version", version))
    LD_ASSERT(LDNodeAddHash(hash, key, flag));

    return flag;
}

static void
testVariationHelper(void (*const prepare)(LDNode **const flag),
    void (*const evaluate)(LDClient *const client))
{
    LDClient *client;
    LDNode *flags, *flag;
    char *serialized;

    LD_ASSERT(!(flags = LDNodeCreateHash()));
    LD_ASSERT(flag = addTestFlag(&flags, "flag-key", 2));
    prepare(&flag);
    LD_ASSERT(serialized = LDHashToJSON(flags));

    LD_ASSERT(client = makeTestClient());
    LDClientRestoreFlags(client, serialized);

    evaluate(client);

    LDFree(serialized);
    LDi_freehash(flags);
    LDClientClose(client);
}

static void
testBoolVariationPrepare(LDNode **const flag)
{
    LD_ASSERT(LDNodeAddBool(flag, "value", true))
}

static void
testBoolVariationEvaluate(LDClient *const client)
{
    LD_ASSERT(LDBoolVariation(client, "flag-key", false));
}

static void
testIntVariationPrepare(LDNode **const flag)
{
    LD_ASSERT(LDNodeAddNumber(flag, "value", 32))
}

static void
testIntVariationEvaluate(LDClient *const client)
{
    LD_ASSERT(LDIntVariation(client, "flag-key", 45) == 32);
}

static void
testIntVariationDoubleFlagPrepare(LDNode **const flag)
{
    LD_ASSERT(LDNodeAddNumber(flag, "value", -1.9))
}

static void
testIntVariationDoubleFlagEvaluate(LDClient *const client)
{
    LD_ASSERT(LDIntVariation(client, "flag-key", 20) == -1);
}

static void
testDoubleVariationPrepare(LDNode **const flag)
{
    LD_ASSERT(LDNodeAddNumber(flag, "value", 32.2))
}

static void
testDoubleVariationEvaluate(LDClient *const client)
{
    LD_ASSERT(LDDoubleVariation(client, "flag-key", 45.2) == 32.2);
}

static void
testStringVariationPrepare(LDNode **const flag)
{
    LD_ASSERT(LDNodeAddString(flag, "value", "hello"))
}

static void
testStringVariationEvaluate(LDClient *const client)
{
    char *result;

    LD_ASSERT(result = LDStringVariationAlloc(client, "flag-key", "world"));
    LD_ASSERT(strcmp(result, "hello") == 0);

    LDFree(result);
}

static void
testJSONVariationPrepare(LDNode **const flag)
{
    LDNode *value;

    LD_ASSERT(!(value = LDNodeCreateHash()));
    LD_ASSERT(LDNodeAddString(&value, "animal", "dog"))

    LD_ASSERT(LDNodeAddHash(flag, "value", value));
}

static void
testJSONVariationEvaluate(LDClient *const client)
{
    LDNode *result;
    char *serialized;

    LD_ASSERT(result = LDJSONVariation(client, "flag-key", NULL));
    LD_ASSERT(serialized = LDHashToJSON(result));
    LD_ASSERT(strcmp(serialized, "{\"animal\":\"dog\"}") == 0);

    LDFree(serialized);
    LDi_freehash(result);
}

int
main()
{
    testVariationHelper(testBoolVariationPrepare,
        testBoolVariationEvaluate);
    testVariationHelper(testIntVariationPrepare,
        testIntVariationEvaluate);
    testVariationHelper(testIntVariationDoubleFlagPrepare,
        testIntVariationDoubleFlagEvaluate);
    testVariationHelper(testDoubleVariationPrepare,
        testDoubleVariationEvaluate);
    testVariationHelper(testStringVariationPrepare,
        testStringVariationEvaluate);
    testVariationHelper(testJSONVariationPrepare,
        testJSONVariationEvaluate);

    return 0;
}
